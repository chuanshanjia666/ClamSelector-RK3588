#ifndef RGA_SCALER_PLUS_RGA_H
#define RGA_SCALER_PLUS_RGA_H

#include "basescaler.h"
#include <rga/RgaApi.h>
#include <rga/rga.h>

extern "C"
{
#include <libavutil/hwcontext_drm.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

class RGAScalerPlus : public BaseScaler {
public:
    RGAScalerPlus(int out_w, int out_h, AVPixelFormat out_fmt,
                 AVBufferRef *hw_device_ctx);
    ~RGAScalerPlus();

    bool Init() override;
    AVFrame *scale_and_convert(AVFrame *hw_frame) override;

private:
    bool init_rga();
    void release_rga();

    // RGA上下文 (根据头文件说明，实际不使用此ctx)
    void* rga_ctx_ = nullptr;  
    AVFrame *dst_frame_ = nullptr;
    AVBufferRef *hw_device_ctx_ = nullptr;
};
#endif // RGA_SCALER_PLUS_RGA_H
