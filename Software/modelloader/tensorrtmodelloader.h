#pragma once
#include "basemodelloader.h"
#include <NvInfer.h>
#include <cuda_runtime_api.h>
#include <string>
#include <vector>
#include <memory>

extern "C"
{
#include <libavutil/frame.h>
}

class TensorRTModelLoader : public BaseModelLoader
{
public:
    TensorRTModelLoader();
    ~TensorRTModelLoader() override;

    bool load_model(const std::string &modelPath) override;
    bool do_inference(const float *input_data, std::vector<float> &output) override;

    // 支持AVFrame输入的推理接口（自动转NCHW）
    bool infer_frame_rgb(const AVFrame *frame, std::vector<float> &output);
    bool infer_frame_gray(const AVFrame *frame, std::vector<float> &output);

    // 打印模型信息
    void show_model_info();

protected:
    // 工具函数：AVFrame转NCHW float张量
    static void avframe_rgb_to_nchw(const AVFrame *frame, int width, int height, int channels, std::vector<float> &out);
    static void avframe_gray_to_nchw(const AVFrame *frame, int width, int height, std::vector<float> &out);

    // 检查帧格式
    static bool check_frame_format_rgb(const AVFrame *frame);
    static bool check_frame_format_gray(const AVFrame *frame);

    // 构建engine（可选扩展）
    bool build_engine(const std::string &enginePath);

    // TensorRT相关成员
    std::unique_ptr<nvinfer1::IRuntime> runtime_;
    std::unique_ptr<nvinfer1::ICudaEngine> engine_;
    std::unique_ptr<nvinfer1::IExecutionContext> context_;

    int input_index_ = 0;
    int output_index_ = 0;
    int input_size_ = 0;
    int output_size_ = 0;
    std::vector<int> input_dims_;
    std::vector<int> output_dims_;
    std::string modelPath_;
};