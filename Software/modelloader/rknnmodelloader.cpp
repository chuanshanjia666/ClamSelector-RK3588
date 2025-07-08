#include "rknnmodelloader.h"
#include <iostream>
#include <fstream>
#include <cstring>

RKNNModelLoader::RKNNModelLoader() : ctx(0) {}

RKNNModelLoader::~RKNNModelLoader()
{
    if (ctx)
    {
        rknn_destroy(ctx);
        ctx = 0;
    }
}

bool RKNNModelLoader::load_model(const std::string &modelPath)
{
    std::ifstream file(modelPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        std::cerr << "Failed to open model file: " << modelPath << std::endl;
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size))
    {
        std::cerr << "Failed to read model file: " << modelPath << std::endl;
        return false;
    }
    int ret = rknn_init(&ctx, buffer.data(), size, 0, nullptr);
    if (ret != 0)
    {
        std::cerr << "rknn_init failed: " << ret << std::endl;
        return false;
    }
    this->modelPath = modelPath;
    std::cout << "[RKNN] Model loaded successfully: " << modelPath << std::endl;
    return update_model_input_attr();
}

bool RKNNModelLoader::update_model_input_attr()
{
    rknn_tensor_attr input_attr;
    memset(&input_attr, 0, sizeof(input_attr));
    input_attr.index = 0;
    int ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &input_attr, sizeof(input_attr));
    if (ret < 0)
    {
        std::cerr << "[RKNN] rknn_query input attr failed: " << ret << std::endl;
        return false;
    }
    input_fmt = input_attr.fmt;
    if (input_fmt == RKNN_TENSOR_NCHW)
    {
        input_width = input_attr.dims[3];
        input_height = input_attr.dims[2];
        input_channels = input_attr.dims[1];
    }
    else
    {
        input_width = input_attr.dims[2];
        input_height = input_attr.dims[1];
        input_channels = input_attr.dims[3];
    }
    input_image_size = input_width * input_height * input_channels;
    std::cout << "[RKNN] Model input attr: width=" << input_width << ", height=" << input_height
              << ", channels=" << input_channels << ", fmt=" << (input_fmt == RKNN_TENSOR_NCHW ? "NCHW" : "NHWC") << std::endl;
    return true;
}

bool RKNNModelLoader::check_frame_format_rgb(const AVFrame *frame)
{
    return frame && frame->format == AV_PIX_FMT_RGB24;
}

bool RKNNModelLoader::check_frame_format_gray(const AVFrame *frame)
{
    return frame && frame->format == AV_PIX_FMT_GRAY8;
}

void RKNNModelLoader::avframe_to_rgb_buffer(const AVFrame *frame, int width, int height, int channels, std::vector<uint8_t> &out)
{
    out.resize(width * height * channels);
    // 添加调试打印
    std::cout << "[DEBUG] avframe_to_rgb_buffer - 参数: "
              << "width=" << width << ", height=" << height
              << ", channels=" << channels << std::endl;
    std::cout << "[DEBUG] AVFrame信息: "
              << "width=" << frame->width << ", height=" << frame->height
              << ", format=" << frame->format << " (AV_PIX_FMT_RGB24=" << AV_PIX_FMT_RGB24 << ")"
              << ", linesize[0]=" << frame->linesize[0] << std::endl;

    out.resize(width * height * channels);
    std::cout << "[DEBUG] 输出缓冲区大小: " << out.size() << " bytes" << std::endl;
    if (input_fmt == RKNN_TENSOR_NCHW)
    {
        // NCHW格式：通道优先排列
        for (int c = 0; c < channels; ++c)
        {
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    out[c * height * width + y * width + x] =
                        frame->data[0][y * frame->linesize[0] + x * channels + c];
                }
            }
        }
    }
    else
    {
        // 默认NHWC格式：像素优先排列
        for (int y = 0; y < height; ++y)
        {
            memcpy(out.data() + y * width * channels,
                   frame->data[0] + y * frame->linesize[0],
                   width * channels);
        }
    }
}

void RKNNModelLoader::avframe_to_gray_buffer(const AVFrame *frame, int width, int height, std::vector<uint8_t> &out)
{
    out.resize(width * height);
    for (int y = 0; y < height; ++y)
    {
        std::memcpy(out.data() + y * width, frame->data[0] + y * frame->linesize[0], width);
    }
    std::cout << "[RKNN] AVFrame copied to GRAY buffer, size=" << out.size() << std::endl;
}

bool RKNNModelLoader::do_inference_debug(const uint8_t *input_data, std::vector<float> &output)
{
    std::cout << "\n[DEBUG] Input tensor values (first 100 elements):\n";
    for (int i = 0; i < std::min(100, input_image_size); ++i)
    {
        printf("%3u ", input_data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");

    std::cout << "[RKNN] do_inference: image_size=" << input_image_size << ", fmt=" << (input_fmt == RKNN_TENSOR_NCHW ? "NCHW" : "NHWC") << std::endl;
    rknn_input rknn_in;
    memset(&rknn_in, 0, sizeof(rknn_in));
    rknn_in.index = 0;
    rknn_in.buf = (void *)input_data;
    rknn_in.size = input_image_size;
    rknn_in.pass_through = 0;
    rknn_in.type = RKNN_TENSOR_UINT8;
    rknn_in.fmt = input_fmt;
    int ret = rknn_inputs_set(ctx, 1, &rknn_in);
    if (ret != 0)
    {
        std::cerr << "[RKNN] rknn_inputs_set failed: " << ret << std::endl;
        return false;
    }
    std::cout << "[RKNN] rknn_inputs_set success." << std::endl;
    ret = rknn_run(ctx, nullptr);
    if (ret != 0)
    {
        std::cerr << "[RKNN] rknn_run failed: " << ret << std::endl;
        return false;
    }
    std::cout << "[RKNN] rknn_run success." << std::endl;
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret < 0)
    {
        std::cerr << "[RKNN] rknn_query input/output num failed: " << ret << std::endl;
        return false;
    }
    std::cout << "[RKNN] Model output num: " << io_num.n_output << std::endl;
    std::vector<rknn_output> outputs(io_num.n_output);
    for (int i = 0; i < static_cast<int>(io_num.n_output); ++i)
    {
        outputs[i].index = i;
        outputs[i].want_float = 1;
        outputs[i].is_prealloc = 0;
        outputs[i].buf = nullptr;
        outputs[i].size = 0;
    }
    ret = rknn_outputs_get(ctx, io_num.n_output, outputs.data(), nullptr);
    if (ret != 0)
    {
        std::cerr << "[RKNN] rknn_outputs_get failed: " << ret << std::endl;
        return false;
    }
    std::cout << "[RKNN] rknn_outputs_get success." << std::endl;
    if (outputs[0].buf && outputs[0].size > 0)
    {
        float *out_ptr = static_cast<float *>(outputs[0].buf);
        output.assign(out_ptr, out_ptr + outputs[0].size / sizeof(float));
        std::cout << "[RKNN] Output[0] size: " << outputs[0].size << ", float count: " << outputs[0].size / sizeof(float) << std::endl;
    }
    rknn_outputs_release(ctx, io_num.n_output, outputs.data());
    std::cout << "[RKNN] rknn_outputs_release done." << std::endl;
    return true;
}

bool RKNNModelLoader::do_inference(const uint8_t *input_data, std::vector<float> &output)
{
    rknn_input rknn_in;
    memset(&rknn_in, 0, sizeof(rknn_in));
    rknn_in.index = 0;
    rknn_in.buf = (void *)input_data;
    rknn_in.size = input_image_size;
    rknn_in.pass_through = 0;
    rknn_in.type = RKNN_TENSOR_UINT8;
    rknn_in.fmt = input_fmt;
    rknn_inputs_set(ctx, 1, &rknn_in);
    rknn_run(ctx, nullptr);
    rknn_input_output_num io_num;
    rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    std::vector<rknn_output> outputs(io_num.n_output);
    for (int i = 0; i < static_cast<int>(io_num.n_output); ++i)
    {
        outputs[i].index = i;
        outputs[i].want_float = 1;
        outputs[i].is_prealloc = 0;
        outputs[i].buf = nullptr;
        outputs[i].size = 0;
    }
    rknn_outputs_get(ctx, io_num.n_output, outputs.data(), nullptr);
    if (outputs[0].buf && outputs[0].size > 0)
        output.assign(static_cast<float *>(outputs[0].buf), static_cast<float *>(outputs[0].buf) + outputs[0].size / sizeof(float));
    rknn_outputs_release(ctx, io_num.n_output, outputs.data());
    return true;
}

bool RKNNModelLoader::infer_frame_rgb(const AVFrame *frame, std::vector<float> &output)
{
    if (!check_frame_format_rgb(frame))
    {
        std::cerr << "[RKNN] Input frame must be AV_PIX_FMT_RGB24!" << std::endl;
        return false;
    }
    std::vector<uint8_t> input_data;
    std::cerr << "[RKNN] Converting AVFrame to RGB buffer..." << std::endl;
    avframe_to_rgb_buffer(frame, input_width, input_height, input_channels, input_data);
    std::cerr << "[RKNN] Converting AVFrame to RGB buffer..." << std::endl;
    std::cout << "[RKNN] Start inference (RGB)..." << std::endl;
    return do_inference_debug(input_data.data(), output);
}

bool RKNNModelLoader::infer_frame_gray(const AVFrame *frame, std::vector<float> &output)
{
    if (!check_frame_format_gray(frame))
    {
        std::cerr << "[RKNN] Input frame must be AV_PIX_FMT_GRAY8!" << std::endl;
        return false;
    }
    std::vector<uint8_t> input_data;
    avframe_to_gray_buffer(frame, input_width, input_height, input_data);
    std::cout << "[RKNN] Start inference (GRAY)..." << std::endl;
    return do_inference_debug(input_data.data(), output);
}

void RKNNModelLoader::show_model_inf()
{
    if (!ctx)
    {
        std::cerr << "[RKNN] Model not loaded, ctx is null!" << std::endl;
        return;
    }

    // 打印输入输出数量
    rknn_input_output_num io_num;
    memset(&io_num, 0, sizeof(io_num));
    int ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret < 0)
    {
        std::cerr << "[RKNN] rknn_query input/output num failed: " << ret << std::endl;
        return;
    }
    std::cout << "======= RKNN Model Info =======" << std::endl;
    std::cout << "Input num: " << io_num.n_input << std::endl;
    std::cout << "Output num: " << io_num.n_output << std::endl;

    // 打印每个输入信息
    for (int i = 0; i < io_num.n_input; ++i)
    {
        rknn_tensor_attr attr;
        memset(&attr, 0, sizeof(attr));
        attr.index = i;
        if (rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &attr, sizeof(attr)) == 0)
        {
            std::cout << "[Input " << i << "] name: " << attr.name
                      << ", dims: [";
            for (int d = 0; d < attr.n_dims; ++d)
            {
                std::cout << attr.dims[d];
                if (d != attr.n_dims - 1)
                    std::cout << ", ";
            }
            std::cout << "], type: " << attr.type
                      << ", fmt: " << (attr.fmt == RKNN_TENSOR_NCHW ? "NCHW" : "NHWC")
                      << ", qnt_type: " << attr.qnt_type
                      << ", qnt_zp: " << attr.zp
                      << ", qnt_scale: " << attr.scale
                      << std::endl;
        }
    }

    // 打印每个输出信息
    for (int i = 0; i < io_num.n_output; ++i)
    {
        rknn_tensor_attr attr;
        memset(&attr, 0, sizeof(attr));
        attr.index = i;
        if (rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &attr, sizeof(attr)) == 0)
        {
            std::cout << "[Output " << i << "] name: " << attr.name
                      << ", dims: [";
            for (int d = 0; d < attr.n_dims; ++d)
            {
                std::cout << attr.dims[d];
                if (d != attr.n_dims - 1)
                    std::cout << ", ";
            }
            std::cout << "], type: " << attr.type
                      << ", fmt: " << (attr.fmt == RKNN_TENSOR_NCHW ? "NCHW" : "NHWC")
                      << ", qnt_type: " << attr.qnt_type
                      << ", qnt_zp: " << attr.zp
                      << ", qnt_scale: " << attr.scale
                      << std::endl;
        }
    }
    std::cout << "===============================" << std::endl;
}