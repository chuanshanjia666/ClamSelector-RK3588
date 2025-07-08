#include "basescaler.h"

BaseScaler::BaseScaler(int in_w, int in_h, AVPixelFormat in_fmt, AVRational in_tb,
                       int out_w, int out_h, AVPixelFormat out_fmt)
    : in_width(in_w), in_height(in_h), in_pix_fmt(in_fmt), in_time_base(in_tb),
      out_width(out_w), out_height(out_h), out_pix_fmt(out_fmt) {}
