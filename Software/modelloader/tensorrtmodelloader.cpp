#include "tensorrtmodelloader.h"
#include <fstream>
#include <iostream>
#include <cassert>

TensorRTModelLoader::TensorRTModelLoader() {}

TensorRTModelLoader::~TensorRTModelLoader()
{
    // 智能指针自动释放
}

bool TensorRTModelLoader::load_model(const std::string &modelPath)
{
    modelPath_ = modelPath;
    std::ifstream file(modelPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        std::cerr << "Failed to open engine file: " << modelPath << std::endl;
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size))
    {
        std::cerr << "Failed to read engine file: " << modelPath << std::endl;
        return false;
    }

    runtime_.reset(nvinfer1::createInferRuntime(gLogger));
    if (!runtime_)
    {
        std::cerr << "Failed to create TensorRT runtime." << std::endl;
        return false;
    }
    engine_.reset(runtime_->deserializeCudaEngine(buffer.data(), size, nullptr));
    if (!engine_)
    {
        std::cerr << "Failed to deserialize engine." << std::endl;
        return false;
    }
    context_.reset(engine_->createExecutionContext());
    if (!context_)
    {
        std::cerr << "Failed to create execution context." << std::endl;
        return false;
    }

    // 获取输入输出信息
    input_index_ = engine_->getBindingIndex(engine_->getBindingName(0));
    output_index_ = engine_->getBindingIndex(engine_->getBindingName(1));
    auto inputDims = engine_->getBindingDimensions(input_index_);
    auto outputDims = engine_->getBindingDimensions(output_index_);
    input_size_ = 1;
    input_dims_.clear();
    for (int i = 0; i < inputDims.nbDims; ++i)
    {
        input_size_ *= inputDims.d[i];
        input_dims_.push_back(inputDims.d[i]);
    }
    output_size_ = 1;
    output_dims_.clear();
    for (int i = 0; i < outputDims.nbDims; ++i)
    {
        output_size_ *= outputDims.d[i];
        output_dims_.push_back(outputDims.d[i]);
    }
    std::cout << "[TensorRT] Engine loaded: " << modelPath << std::endl;
    return true;
}

bool TensorRTModelLoader::check_frame_format_rgb(const AVFrame *frame)
{
    return frame && frame->format == AV_PIX_FMT_RGB24;
}

bool TensorRTModelLoader::check_frame_format_gray(const AVFrame *frame)
{
    return frame && frame->format == AV_PIX_FMT_GRAY8;
}

void TensorRTModelLoader::avframe_rgb_to_nchw(const AVFrame *frame, int width, int height, int channels, std::vector<float> &out)
{
    out.resize(width * height * channels);
    for (int c = 0; c < channels; ++c)
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int src_idx = y * frame->linesize[0] + x * channels + c;
                int dst_idx = c * width * height + y * width + x;
                out[dst_idx] = static_cast<float>(frame->data[0][src_idx]) / 255.0f;
            }
        }
    }
}

void TensorRTModelLoader::avframe_gray_to_nchw(const AVFrame *frame, int width, int height, std::vector<float> &out)
{
    out.resize(width * height);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int src_idx = y * frame->linesize[0] + x;
            int dst_idx = y * width + x;
            out[dst_idx] = static_cast<float>(frame->data[0][src_idx]) / 255.0f;
        }
    }
}

bool TensorRTModelLoader::infer_frame_rgb(const AVFrame *frame, std::vector<float> &output)
{
    if (!check_frame_format_rgb(frame))
    {
        std::cerr << "[TensorRT] Input frame must be AV_PIX_FMT_RGB24!" << std::endl;
        return false;
    }
    int channels = input_dims_.size() > 0 ? input_dims_[0] : 3;
    int height = input_dims_.size() > 1 ? input_dims_[1] : frame->height;
    int width = input_dims_.size() > 2 ? input_dims_[2] : frame->width;
    std::vector<float> nchw_data;
    avframe_rgb_to_nchw(frame, width, height, channels, nchw_data);
    std::cout << "[TensorRT] Start inference (RGB)..." << std::endl;
    return do_inference_debug(nchw_data.data(), output);
}

bool TensorRTModelLoader::infer_frame_gray(const AVFrame *frame, std::vector<float> &output)
{
    if (!check_frame_format_gray(frame))
    {
        std::cerr << "[TensorRT] Input frame must be AV_PIX_FMT_GRAY8!" << std::endl;
        return false;
    }
    int height = input_dims_.size() > 1 ? input_dims_[1] : frame->height;
    int width = input_dims_.size() > 2 ? input_dims_[2] : frame->width;
    std::vector<float> nchw_data;
    avframe_gray_to_nchw(frame, width, height, nchw_data);
    std::cout << "[TensorRT] Start inference (GRAY)..." << std::endl;
    return do_inference_debug(nchw_data.data(), output);
}

bool TensorRTModelLoader::do_inference_debug(const float *input_data, std::vector<float> &output)
{
    if (!context_)
    {
        std::cerr << "[TensorRT] context_ is null!" << std::endl;
        return false;
    }
    void *buffers[2] = {nullptr, nullptr};
    if (cudaMalloc(&buffers[input_index_], input_size_ * sizeof(float)) != cudaSuccess)
    {
        std::cerr << "[TensorRT] cudaMalloc input failed!" << std::endl;
        return false;
    }
    if (cudaMalloc(&buffers[output_index_], output_size_ * sizeof(float)) != cudaSuccess)
    {
        std::cerr << "[TensorRT] cudaMalloc output failed!" << std::endl;
        cudaFree(buffers[input_index_]);
        return false;
    }
    if (cudaMemcpy(buffers[input_index_], input_data, input_size_ * sizeof(float), cudaMemcpyHostToDevice) != cudaSuccess)
    {
        std::cerr << "[TensorRT] cudaMemcpy input failed!" << std::endl;
        cudaFree(buffers[input_index_]);
        cudaFree(buffers[output_index_]);
        return false;
    }

    bool ok = context_->enqueueV2(buffers, 0, nullptr);
    if (!ok)
    {
        std::cerr << "[TensorRT] enqueueV2 failed!" << std::endl;
        cudaFree(buffers[input_index_]);
        cudaFree(buffers[output_index_]);
        return false;
    }

    output.resize(output_size_);
    if (cudaMemcpy(output.data(), buffers[output_index_], output_size_ * sizeof(float), cudaMemcpyDeviceToHost) != cudaSuccess)
    {
        std::cerr << "[TensorRT] cudaMemcpy output failed!" << std::endl;
        cudaFree(buffers[input_index_]);
        cudaFree(buffers[output_index_]);
        return false;
    }

    cudaFree(buffers[input_index_]);
    cudaFree(buffers[output_index_]);
    std::cout << "[TensorRT] Inference done, output size: " << output.size() << std::endl;
    return true;
}

bool TensorRTModelLoader::do_inference(const float *input_data, std::vector<float> &output)
{
    if (!context_)
        return false;
    void *buffers[2] = {nullptr, nullptr};
    cudaMalloc(&buffers[input_index_], input_size_ * sizeof(float));
    cudaMalloc(&buffers[output_index_], output_size_ * sizeof(float));
    cudaMemcpy(buffers[input_index_], input_data, input_size_ * sizeof(float), cudaMemcpyHostToDevice);
    context_->enqueueV2(buffers, 0, nullptr);
    output.resize(output_size_);
    cudaMemcpy(output.data(), buffers[output_index_], output_size_ * sizeof(float), cudaMemcpyDeviceToHost);
    cudaFree(buffers[input_index_]);
    cudaFree(buffers[output_index_]);
    return true;
}

void TensorRTModelLoader::show_model_info()
{
    if (!engine_)
    {
        std::cout << "No engine loaded." << std::endl;
        return;
    }
    std::cout << "TensorRT Engine Info:" << std::endl;
    std::cout << "Input dims: ";
    for (auto d : input_dims_)
        std::cout << d << " ";
    std::cout << std::endl;
    std::cout << "Output dims: ";
    for (auto d : output_dims_)
        std::cout << d << " ";
    std::cout << std::endl;
}