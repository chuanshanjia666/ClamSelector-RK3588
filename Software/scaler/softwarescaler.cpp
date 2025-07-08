#include "softwarescaler.h"



SoftwareScaler::SoftwareScaler(int in_w, int in_h, AVPixelFormat in_fmt, AVRational time_base,
                               int out_w, int out_h, AVPixelFormat out_fmt)
    : BaseScaler(in_w, in_h, in_fmt, time_base, out_w, out_h, out_fmt),
      filter_graph(nullptr), buffersrc_ctx(nullptr), buffersink_ctx(nullptr)
{
}

bool SoftwareScaler::Init()
{
    char args[256];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=1/1",
             in_width, in_height, in_pix_fmt, in_time_base.num, in_time_base.den);

    filter_graph = avfilter_graph_alloc();
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *scale = avfilter_get_by_name("scale");
    const AVFilter *format = avfilter_get_by_name("format");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, nullptr, filter_graph);

    AVFilterContext *scale_ctx = nullptr;
    char scale_args[64];
    snprintf(scale_args, sizeof(scale_args), "w=%d:h=%d", out_width, out_height);
    avfilter_graph_create_filter(&scale_ctx, scale, "scale", scale_args, nullptr, filter_graph);

    AVFilterContext *format_ctx = nullptr;
    char fmt_args[64];
    snprintf(fmt_args, sizeof(fmt_args), "pix_fmts=%s", av_get_pix_fmt_name(out_pix_fmt));
    avfilter_graph_create_filter(&format_ctx, format, "format", fmt_args, nullptr, filter_graph);

    avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", nullptr, nullptr, filter_graph);

    avfilter_link(buffersrc_ctx, 0, scale_ctx, 0);
    avfilter_link(scale_ctx, 0, format_ctx, 0);
    avfilter_link(format_ctx, 0, buffersink_ctx, 0);

    if (avfilter_graph_config(filter_graph, nullptr) < 0)
    {
        std::cerr << "Failed to config filter graph!" << std::endl;
    }

    return true;
}

SoftwareScaler::~SoftwareScaler()
{
    avfilter_graph_free(&filter_graph);
}

AVFrame *SoftwareScaler::scale_and_convert(AVFrame *in)
{
    if (av_buffersrc_add_frame(buffersrc_ctx, in) < 0)
    {
        std::cerr << "Error feeding the filter graph" << std::endl;
        return nullptr;
    }
    AVFrame *out = av_frame_alloc();
    if (av_buffersink_get_frame(buffersink_ctx, out) >= 0)
    {
        return out;
    }
    else
    {
        av_frame_free(&out);
        return nullptr;
    }
}
