#include "basemodelloader.h"
#include <iostream>

bool BaseModelLoader::do_inference(const uint8_t *input_data, std::vector<float> &output)
{
    // This is a placeholder implementation. Actual inference logic should be implemented in derived classes.
    std::cerr << "BaseModelLoader::do_inference not implemented." << std::endl;
    return false;
}

bool BaseModelLoader::do_inference(const float *input_data, std::vector<float> &output)
{
    // This is a placeholder implementation. Actual inference logic should be implemented in derived classes.
    std::cerr << "BaseModelLoader::do_inference with float input not implemented." << std::endl;
    return false;
}

bool BaseModelLoader::do_inference_debug(const uint8_t *input_data, std::vector<float> &output)
{
    // This is a placeholder implementation. Actual inference logic should be implemented in derived classes.
    std::cerr << "BaseModelLoader::do_inference_debug not implemented." << std::endl;
    return false;
}

bool BaseModelLoader::do_inference_debug(const float *input_data, std::vector<float> &output)
{
    // This is a placeholder implementation. Actual inference logic should be implemented in derived classes.
    std::cerr << "BaseModelLoader::do_inference_debug with float input not implemented." << std::endl;
    return false;
}
