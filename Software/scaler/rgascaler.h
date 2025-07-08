#ifndef __GRASCALERLH
#define __GRASCALERLH

#include "basescaler.h"

extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include<libavfilter/avfilter.h>
#include<libavfilter/buffersrc.h>
#include<libavfilter/buffersink.h>
}

class RGAScaler : public BaseScaler
{
public:
    // 强制要求传入硬件上下文（不再提供默认构造）
    RGAScaler(int in_w, int in_h, AVPixelFormat in_fmt,
              int out_w, int out_h, AVPixelFormat out_fmt,
              AVBufferRef *hw_device_ctx); // 必须传入有效的硬件上下文

    ~RGAScaler();
    bool Init() override;
    AVFrame *scale_and_convert(AVFrame *in) override;

private:
    AVFilterGraph *filter_graph = nullptr;
    AVFilterContext *buffersrc_ctx = nullptr;
    AVFilterContext *buffersink_ctx = nullptr;
    AVBufferRef *hw_device_ctx; // 直接持有引用（不管理生命周期）
};

#endif