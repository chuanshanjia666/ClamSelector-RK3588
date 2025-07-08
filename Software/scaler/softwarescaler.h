#ifndef SOFTWARESCALER_H
#define SOFTWARESCALER_H
#include "basescaler.h"
#include <iostream>
extern "C"
{
#include <libavutil/pixfmt.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
}
class SoftwareScaler : public BaseScaler
{
public:
    SoftwareScaler(int in_w, int in_h, AVPixelFormat in_fmt, AVRational time_base,
                   int out_w, int out_h, AVPixelFormat out_fmt);
    ~SoftwareScaler();
    AVFrame *scale_and_convert(AVFrame *in) override;
    bool Init() override;

private:
    AVFilterGraph *filter_graph= nullptr;
    AVFilterContext *buffersrc_ctx = nullptr;
    AVFilterContext *buffersink_ctx = nullptr;
};
#endif // SOFTWARESCALER_H
