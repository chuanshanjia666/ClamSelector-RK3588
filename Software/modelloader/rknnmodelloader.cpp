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
    return update_model_io_attr();
}

bool RKNNModelLoader::update_model_io_attr()
{
    // 查询IO数量
    int ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret < 0)
    {
        std::cerr << "[RKNN] rknn_query input/output num failed: " << ret << std::endl;
        return false;
    }

    // 查询输入属性
    rknn_tensor_attr input_attr;
    memset(&input_attr, 0, sizeof(input_attr));
    input_attr.index = 0;
    ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &input_attr, sizeof(input_attr));
    if (ret < 0)
    {
        std::cerr << "[RKNN] rknn_query input attr failed: " << ret << std::endl;
        return false;
    }

    // 保存输入属性
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
    input_type = input_attr.type;
    input_qnt_type = input_attr.qnt_type;
    input_scale = input_attr.scale;
    input_zp = input_attr.zp;

    // 查询并保存输出属性
    output_num = io_num.n_output;
    output_attrs.resize(output_num);
    for (int i = 0; i < output_num; ++i)
    {
        memset(&output_attrs[i], 0, sizeof(rknn_tensor_attr));
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &output_attrs[i], sizeof(rknn_tensor_attr));
        if (ret < 0)
        {
            std::cerr << "[RKNN] rknn_query output attr failed for output " << i << ": " << ret << std::endl;
            return false;
        }
    }

    std::cout << "[RKNN] Model input attr: width=" << input_width << ", height=" << input_height
              << ", channels=" << input_channels << ", fmt=" << (input_fmt == RKNN_TENSOR_NCHW ? "NCHW" : "NHWC")
              << ", type=" << input_type << ", qnt_type=" << input_qnt_type
              << ", scale=" << input_scale << ", zp=" << input_zp << std::endl;
    show_model_inf();
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

void RKNNModelLoader::avframe_to_rgb_buffer(const AVFrame *frame, int width, int height, int channels, std::vector<int16_t> &out)
{
    out.resize(width * height * channels);
    std::cout << "[DEBUG] Converting AVFrame to INT16 RGB buffer\n";
    std::cout << "[DEBUG] Output buffer size: " << out.size() * sizeof(int16_t) << " bytes\n";

    if (input_fmt == RKNN_TENSOR_NCHW)
    {
        // NCHW 格式转换
        for (int c = 0; c < channels; ++c)
        {
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    // uint8 -> int16 转换，并应用减128的偏移（常见处理）
                    out[c * height * width + y * width + x] =
                        static_cast<int16_t>(frame->data[0][y * frame->linesize[0] + x * channels + c]) - 128;
                }
            }
        }
    }
    else
    {
        // NHWC 格式转换
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                for (int c = 0; c < channels; ++c)
                {
                    out[y * width * channels + x * channels + c] =
                        static_cast<int16_t>(frame->data[0][y * frame->linesize[0] + x * channels + c]) - 128;
                }
            }
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
    std::cout << "[RKNN] do_inference_debug:\n"
              << "  image_size=" << input_image_size << "\n"
              << "  fmt=" << (input_fmt == RKNN_TENSOR_NCHW ? "NCHW" : "NHWC") << "\n"
              << "  input_type=" << input_type << "\n"
              << "  qnt_type=" << input_qnt_type << "\n"
              << "  scale=" << input_scale << "\n"
              << "  zp=" << input_zp << std::endl;

    // 设置输入数据 - 直接使用uint8数据，让RKNN库自动处理量化
    rknn_input rknn_in;
    memset(&rknn_in, 0, sizeof(rknn_in));
    rknn_in.index = 0;
    rknn_in.buf = (void *)input_data;
    rknn_in.size = input_image_size;
    rknn_in.pass_through = 0;         // 让RKNN库自动处理量化
    rknn_in.type = RKNN_TENSOR_UINT8; // 输入固定为uint8
    rknn_in.fmt = input_fmt;

    // 设置输入
    int ret = rknn_inputs_set(ctx, 1, &rknn_in);
    if (ret != RKNN_SUCC)
    {
        std::cerr << "[RKNN] rknn_inputs_set failed: " << ret << std::endl;
        return false;
    }
    std::cout << "[RKNN] rknn_inputs_set success." << std::endl;

    // 执行推理
    ret = rknn_run(ctx, nullptr);
    if (ret != RKNN_SUCC)
    {
        std::cerr << "[RKNN] rknn_run failed: " << ret << std::endl;
        return false;
    }
    std::cout << "[RKNN] rknn_run success." << std::endl;

    // 获取输出 - 直接要求float输出，让RKNN库自动处理反量化
    std::vector<rknn_output> outputs(io_num.n_output);
    for (int i = 0; i < io_num.n_output; ++i)
    {
        outputs[i].index = i;
        outputs[i].want_float = 1; // 要求float输出，RKNN库自动反量化
    }

    // 获取输出
    ret = rknn_outputs_get(ctx, io_num.n_output, outputs.data(), nullptr);
    if (ret != RKNN_SUCC)
    {
        std::cerr << "[RKNN] rknn_outputs_get failed: " << ret << std::endl;
        return false;
    }
    std::cout << "[RKNN] rknn_outputs_get success." << std::endl;

    // 处理输出数据
    if (outputs[0].buf && outputs[0].size > 0)
    {
        float *raw_output = static_cast<float *>(outputs[0].buf);
        output.assign(raw_output, raw_output + outputs[0].size / sizeof(float));

        // 调试打印
        std::cout << "[RKNN] Output details:\n"
                  << "  Size: " << outputs[0].size << " bytes\n"
                  << "  Values (first 5): ";
        for (int i = 0; i < std::min(5, (int)output.size()); ++i)
        {
            printf("%.6f ", output[i]);
        }
        printf("\n");
    }

    // 释放输出
    rknn_outputs_release(ctx, io_num.n_output, outputs.data());
    std::cout << "[RKNN] rknn_outputs_release done." << std::endl;

    return true;
}

bool RKNNModelLoader::do_inference(const uint8_t *input_data, std::vector<float> &output)
{
    // 设置输入数据 - 直接使用uint8数据，让RKNN库自动处理量化
    rknn_input rknn_in;
    memset(&rknn_in, 0, sizeof(rknn_in));
    rknn_in.index = 0;
    rknn_in.buf = (void *)input_data;
    rknn_in.size = input_image_size;
    rknn_in.pass_through = 0;         // 让RKNN库自动处理量化
    rknn_in.type = RKNN_TENSOR_UINT8; // 输入固定为uint8
    rknn_in.fmt = input_fmt;

    // 执行推理
    if (rknn_inputs_set(ctx, 1, &rknn_in) != RKNN_SUCC)
    {
        std::cerr << "[RKNN] rknn_inputs_set failed." << std::endl;
        return false;
    }

    if (rknn_run(ctx, nullptr) != RKNN_SUCC)
    {
        std::cerr << "[RKNN] rknn_run failed." << std::endl;
        return false;
    }

    // 获取输出 - 直接要求float输出，让RKNN库自动处理反量化
    std::vector<rknn_output> outputs(io_num.n_output);
    for (int i = 0; i < io_num.n_output; ++i)
    {
        outputs[i].index = i;
        outputs[i].want_float = 1; // 要求float输出，RKNN库自动反量化
    }

    if (rknn_outputs_get(ctx, io_num.n_output, outputs.data(), nullptr) != RKNN_SUCC)
    {
        std::cerr << "[RKNN] rknn_outputs_get failed." << std::endl;
        return false;
    }

    // 复制输出数据
    if (outputs[0].buf && outputs[0].size > 0)
    {
        float *raw_output = static_cast<float *>(outputs[0].buf);
        output.assign(raw_output, raw_output + outputs[0].size / sizeof(float));
    }

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
    avframe_to_rgb_buffer(frame, input_width, input_height, input_channels, input_data);
    return do_inference(input_data.data(), output);
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
    return do_inference(input_data.data(), output);
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
