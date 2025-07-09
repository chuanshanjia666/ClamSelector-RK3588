#include "rgascaler.h"


RGAScaler::RGAScaler(int in_w, int in_h, AVPixelFormat in_fmt,
                     int out_w, int out_h, AVPixelFormat out_fmt,
                     AVBufferRef *hw_device_ctx)
    : BaseScaler(in_w, in_h, in_fmt, {1, 1}, out_w, out_h, out_fmt),
      hw_device_ctx(hw_device_ctx)
{ // 直接保存外部上下文
    
}

RGAScaler::~RGAScaler()
{
    if (filter_graph)
    {
        avfilter_graph_free(&filter_graph);
    }
    // 注意：不释放 hw_device_ctx，由外部管理
}

bool RGAScaler::Init()
{
    // 1. 创建滤镜图
    filter_graph = avfilter_graph_alloc();
    if (!filter_graph)
        return false;

    // 2. 配置输入滤镜（CPU内存）
    char src_args[256];
    snprintf(src_args, sizeof(src_args),
             "video_size=%dx%d:pix_fmt=%d:time_base=1/1",
             in_width, in_height, in_pix_fmt);

    if (avfilter_graph_create_filter(&buffersrc_ctx,
                                     avfilter_get_by_name("buffer"),
                                     "in", src_args, NULL, filter_graph) < 0)
    {
        return false;
    }

    // 3. 配置输出滤镜
    if (avfilter_graph_create_filter(&buffersink_ctx,
                                     avfilter_get_by_name("buffersink"),
                                     "out", NULL, NULL, filter_graph) < 0)
    {
        return false;
    }

    // 4. 设置输出格式
    enum AVPixelFormat pix_fmts[] = {out_pix_fmt, AV_PIX_FMT_NONE};
    if (av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                            AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0)
    {
        return false;
    }

    // 5. 构建RGA滤镜链（自动绑定硬件上下文）
    const char *filter_descr = "hwupload,scale_rkrga=w=%d:h=%d:format=%d,hwdownload";
    char filter_str[256];
    snprintf(filter_str, sizeof(filter_str), filter_descr,
             out_width, out_height, out_pix_fmt);

    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;

    if (avfilter_graph_parse_ptr(filter_graph, filter_str,
                                 &inputs, &outputs, NULL) < 0)
    {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return false;
    }

    // 6. 显式绑定硬件上下文到hwupload
    AVFilterContext *hwupload_ctx = avfilter_graph_get_filter(filter_graph, "Parsed_hwupload_0");
    if (hwupload_ctx)
    {
        hwupload_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx); // 绑定共享的上下文
    }
    else
    {
        return false; // 必须存在hwupload节点
    }

    // 7. 最终配置
    if (avfilter_graph_config(filter_graph, NULL) < 0)
    {
        return false;
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return true;
}

AVFrame *RGAScaler::scale_and_convert(AVFrame *in)
{
    if (!in || in->format != in_pix_fmt)
        return nullptr;

    if (av_buffersrc_add_frame_flags(buffersrc_ctx, in,
                                     AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
    {
        return nullptr;
    }

    AVFrame *out = av_frame_alloc();
    if (av_buffersink_get_frame(buffersink_ctx, out) < 0)
    {
        av_frame_free(&out);
        return nullptr;
    }
    return out;
}
