#include"basedecoder.hpp"
#include <iostream>
extern "C"
{
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
}

void BaseDecoder::printFrameInfo(AVFrame *frame)
{
    if (!frame)
    {
        std::cerr << "Frame is null!" << std::endl;
        return;
    }

    std::cout << "\n======= Frame Info =======" << std::endl;
    std::cout << "Decode type: " << (frame->hw_frames_ctx ? "Hardware" : "Software") << std::endl;

    if (frame->hw_frames_ctx)
    {
        AVHWFramesContext *hw_frames_ctx = (AVHWFramesContext *)frame->hw_frames_ctx->data;
        std::cout << "HW frame storage type: " << av_hwdevice_get_type_name(hw_frames_ctx->device_ctx->type) << std::endl;
    }

    std::cout << "Pixel format: " << av_get_pix_fmt_name((AVPixelFormat)frame->format) << std::endl;
    std::cout << "Resolution: " << frame->width << "x" << frame->height << std::endl;
    std::cout << "Frame type: " << av_get_picture_type_char(frame->pict_type) << std::endl;
    std::cout << "PTS: " << frame->pts << std::endl;
    std::cout << "===========================" << std::endl;
}