#pragma once
#include <string>
#include <vector>
extern "C"
{
#include <libavutil/frame.h>
}

class BaseModelLoader
{
protected:
    std::string modelPath;
    virtual bool do_inference_debug(const uint8_t *input_data, std::vector<float> &output);
    virtual bool do_inference_debug(const float *input_data, std::vector<float> &output);
    virtual bool do_inference(const uint8_t *input_data, std::vector<float> &output);
    virtual bool do_inference(const float *input_data, std::vector<float> &output);

public:
    BaseModelLoader() = default;
    virtual ~BaseModelLoader() = default;
    virtual bool load_model(const std::string &modelPath) = 0;

};
