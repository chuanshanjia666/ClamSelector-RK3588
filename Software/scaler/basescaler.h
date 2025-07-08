#ifndef BASESCALER_H
#define BASESCALER_H

extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>
}

class BaseScaler
{
public:
    BaseScaler(int in_w, int in_h, AVPixelFormat in_fmt, AVRational in_tb,
               int out_w, int out_h, AVPixelFormat out_fmt);
    virtual ~BaseScaler() {}
    virtual bool Init() = 0;
    virtual AVFrame *scale_and_convert(AVFrame *in) = 0;

protected:
    int in_width;
    int in_height;
    AVPixelFormat in_pix_fmt;
    AVRational in_time_base;
    int out_width;
    int out_height;
    AVPixelFormat out_pix_fmt;
};

#endif // BASESCALER_H
