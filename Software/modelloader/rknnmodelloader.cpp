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
    // // 添加调试打印
    // std::cout << "[DEBUG] avframe_to_rgb_buffer - 参数: "
    //           << "width=" << width << ", height=" << height
    //           << ", channels=" << channels << std::endl;
    // std::cout << "[DEBUG] AVFrame信息: "
    //           << "width=" << frame->width << ", height=" << frame->height
    //           << ", format=" << frame->format << " (AV_PIX_FMT_RGB24=" << AV_PIX_FMT_RGB24 << ")"
    //           << ", linesize[0]=" << frame->linesize[0] << std::endl;

    out.resize(width * height * channels);
    // std::cout << "[DEBUG] 输出缓冲区大小: " << out.size() << " bytes" << std::endl;
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
    // // 打印输入数据
    // std::cout << "\n[DEBUG] Input tensor values (first 100 elements):\n";
    // for (int i = 0; i < std::min(100, input_image_size); ++i)
    // {
    //     printf("%3u ", input_data[i]);
    //     if ((i + 1) % 16 == 0)
    //         printf("\n");
    // }
    // printf("\n");

    std::cout << "[RKNN] do_inference_debug:\n"
              << "  image_size=" << input_image_size << "\n"
              << "  fmt=" << (input_fmt == RKNN_TENSOR_NCHW ? "NCHW" : "NHWC") << "\n"
              << "  input_type=" << input_type << "\n"
              << "  qnt_type=" << input_qnt_type << "\n"
              << "  scale=" << input_scale << "\n"
              << "  zp=" << input_zp << std::endl;

    // 设置输入(使用成员变量)
    rknn_input rknn_in;
    memset(&rknn_in, 0, sizeof(rknn_in));
    rknn_in.index = 0;
    rknn_in.buf = (void *)input_data;
    rknn_in.size = input_image_size;
    rknn_in.pass_through = 0;  // 启用自动量化处理
    rknn_in.type = input_type; // 使用成员变量
    rknn_in.fmt = input_fmt;   // 使用成员变量

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

    // 准备输出缓冲区(使用成员变量io_num)
    std::vector<rknn_output> outputs(io_num.n_output);
    for (int i = 0; i < io_num.n_output; ++i)
    {
        outputs[i].index = i;
        outputs[i].want_float = 1; // 直接获取浮点输出
    }

    // 获取输出
    ret = rknn_outputs_get(ctx, io_num.n_output, outputs.data(), nullptr);
    if (ret != RKNN_SUCC)
    {
        std::cerr << "[RKNN] rknn_outputs_get failed: " << ret << std::endl;
        return false;
    }
    std::cout << "[RKNN] rknn_outputs_get success." << std::endl;

    // [7] 处理第一个输出
    if (outputs[0].buf && outputs[0].size > 0)
    {
        float *raw_output = static_cast<float *>(outputs[0].buf);
        output.assign(raw_output, raw_output + outputs[0].size / sizeof(float));

        // [8] 调用成员函数进行反量化
        dequantize_output(output, 0); // 对第0个输出进行反量化

        // [9] 调试打印
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

// bool RKNNModelLoader::do_inference(const uint8_t *input_data, std::vector<float> &output)
// {
//     // rknn_input rknn_in;
//     // memset(&rknn_in, 0, sizeof(rknn_in));
//     // rknn_in.index = 0;
//     // rknn_in.buf = (void *)input_data;
//     // rknn_in.size = input_image_size;
//     // rknn_in.pass_through = 0;
//     // rknn_in.type = RKNN_TENSOR_UINT8;
//     // rknn_in.fmt = input_fmt;
//     // rknn_inputs_set(ctx, 1, &rknn_in);
//     // rknn_run(ctx, nullptr);
//     // rknn_input_output_num io_num;
//     // rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
//     // std::vector<rknn_output> outputs(io_num.n_output);
//     // for (int i = 0; i < static_cast<int>(io_num.n_output); ++i)
//     // {
//     //     outputs[i].index = i;
//     //     outputs[i].want_float = 1;
//     //     outputs[i].is_prealloc = 0;
//     //     outputs[i].buf = nullptr;
//     //     outputs[i].size = 0;
//     // }
//     // rknn_outputs_get(ctx, io_num.n_output, outputs.data(), nullptr);
//     // if (outputs[0].buf && outputs[0].size > 0)
//     //     output.assign(static_cast<float *>(outputs[0].buf), static_cast<float *>(outputs[0].buf) + outputs[0].size / sizeof(float));
//     // rknn_outputs_release(ctx, io_num.n_output, outputs.data());
//     // return true;

//     rknn_input rknn_in;
//     memset(&rknn_in, 0, sizeof(rknn_in));
//     rknn_in.index = 0;
//     rknn_in.buf = (void *)input_data;
//     rknn_in.size = input_image_size;
//     rknn_in.pass_through = 0;
//     rknn_in.type = input_type; // 用成员变量确保与模型匹配
//     rknn_in.fmt = input_fmt;

//     if (rknn_inputs_set(ctx, 1, &rknn_in) != RKNN_SUCC)
//     {
//         std::cerr << "[RKNN] rknn_inputs_set failed." << std::endl;
//         return false;
//     }

//     if (rknn_run(ctx, nullptr) != RKNN_SUCC)
//     {
//         std::cerr << "[RKNN] rknn_run failed." << std::endl;
//         return false;
//     }

//     std::vector<rknn_output> outputs(io_num.n_output);
//     for (int i = 0; i < static_cast<int>(io_num.n_output); ++i)
//     {
//         outputs[i].index = i;
//         outputs[i].want_float = 1;
//     }

//     if (rknn_outputs_get(ctx, io_num.n_output, outputs.data(), nullptr) != RKNN_SUCC)
//     {
//         std::cerr << "[RKNN] rknn_outputs_get failed." << std::endl;
//         return false;
//     }

//     if (outputs[0].buf && outputs[0].size > 0)
//     {
//         float *raw_output = static_cast<float *>(outputs[0].buf);
//         output.assign(raw_output, raw_output + outputs[0].size / sizeof(float));

//         // 添加反量化
//         dequantize_output(output, 0);
//     }

//     rknn_outputs_release(ctx, io_num.n_output, outputs.data());
//     return true;
// }

bool RKNNModelLoader::do_inference(const uint8_t *input_data, std::vector<float> &output)
{
    // 对输入数据进行量化（正向量化）
    std::vector<int8_t> quantized_data(input_image_size);
    for (size_t i = 0; i < input_image_size; ++i)
    {
        // 假设模型的输入是 UINT8 数据，需要将其量化为 INT8 数据
        quantized_data[i] = static_cast<int8_t>(
            std::round((input_data[i] - input_zp) / input_scale) // 应用量化公式
        );
    }

    // 设置输入数据
    rknn_input rknn_in;
    memset(&rknn_in, 0, sizeof(rknn_in));
    rknn_in.index = 0;
    rknn_in.buf = quantized_data.data(); // 使用量化后的数据
    rknn_in.size = quantized_data.size();
    rknn_in.type = RKNN_TENSOR_INT8; // 量化后的数据类型是 INT8
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

    // 获取输出并进行反量化
    return get_and_dequantize_output(output);
}

bool RKNNModelLoader::get_and_dequantize_output(std::vector<float> &output)
{
    std::vector<rknn_output> outputs(io_num.n_output);
    for (int i = 0; i < io_num.n_output; ++i)
    {
        outputs[i].index = i;
        outputs[i].want_float = 1; // 告诉 RKNN 自动反量化
    }

    if (rknn_outputs_get(ctx, io_num.n_output, outputs.data(), nullptr) != RKNN_SUCC)
    {
        std::cerr << "[RKNN] rknn_outputs_get failed." << std::endl;
        return false;
    }

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
    // std::cerr << "[RKNN] Converting AVFrame to RGB buffer..." << std::endl;
    avframe_to_rgb_buffer(frame, input_width, input_height, input_channels, input_data);
    // std::cerr << "[RKNN] Converting AVFrame to RGB buffer..." << std::endl;
    // std::cout << "[RKNN] Start inference (RGB)..." << std::endl;
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

void RKNNModelLoader::dequantize_output(std::vector<float> &output, int output_index)
{
    if (output_index < 0 || output_index >= output_num)
    {
        std::cerr << "[RKNN] Invalid output index for dequantization: " << output_index << std::endl;
        return;
    }

    const auto &attr = output_attrs[output_index];
    if (attr.qnt_type != RKNN_TENSOR_QNT_NONE)
    {
        float scale = attr.scale;
        for (auto &val : output)
        {
            val *= scale;
        }
    }
}