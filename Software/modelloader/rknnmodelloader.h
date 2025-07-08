#pragma once
#include "basemodelloader.h"
#include <string>
#include <vector>
extern "C"
{
#include <libavutil/frame.h>
}
#include "rknn_api.h"

class RKNNModelLoader : public BaseModelLoader
{
public:
    RKNNModelLoader();
    ~RKNNModelLoader() override;

    bool load_model(const std::string &modelPath) override;
    void show_model_inf();
    // 推理接口
    bool infer_frame_rgb(const AVFrame *frame, std::vector<float> &output);
    bool infer_frame_gray(const AVFrame *frame, std::vector<float> &output);

protected:
    // 工具函数
    static bool check_frame_format_rgb(const AVFrame *frame);
    static bool check_frame_format_gray(const AVFrame *frame);
    void avframe_to_rgb_buffer(const AVFrame *frame, int width, int height, int channels, std::vector<uint8_t> &out);
    void avframe_to_gray_buffer(const AVFrame *frame, int width, int height, std::vector<uint8_t> &out);

    // 内部推理
    bool do_inference(const uint8_t *input_data, std::vector<float> &output) override;
    bool do_inference_debug(const uint8_t *input_data, std::vector<float> &output) override;
    // 模型输入属性
    bool update_model_io_attr();
    void dequantize_output(std::vector<float> &output, int output_index);
    rknn_context ctx = 0;

    // 输入属性
    int input_width = 0;
    int input_height = 0;
    int input_channels = 0;
    int input_image_size = 0;
    rknn_tensor_format input_fmt = RKNN_TENSOR_NHWC;
    rknn_tensor_type input_type = RKNN_TENSOR_UINT8;
    rknn_tensor_qnt_type input_qnt_type = RKNN_TENSOR_QNT_NONE;
    float input_scale = 1.0f;
    int input_zp = 0;

    // 输出属性
    int output_num = 0;
    std::vector<rknn_tensor_attr> output_attrs;

    // IO数量
    rknn_input_output_num io_num;
};