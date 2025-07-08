#include "rknnmodelloader.h"
#include <iostream>
#include <vector>
#include <string>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

// 支持解码为RGB24或灰度
AVFrame *decode_image(const std::string &img_path, AVPixelFormat dst_fmt)
{
    AVFormatContext *fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, img_path.c_str(), nullptr, nullptr) < 0)
    {
        std::cerr << "无法打开图片: " << img_path << std::endl;
        return nullptr;
    }
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0)
    {
        std::cerr << "无法获取图片流信息" << std::endl;
        avformat_close_input(&fmt_ctx);
        return nullptr;
    }
    const AVCodec *codec = nullptr;
    int stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (stream_idx < 0)
    {
        std::cerr << "未找到视频流" << std::endl;
        avformat_close_input(&fmt_ctx);
        return nullptr;
    }
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[stream_idx]->codecpar);
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0)
    {
        std::cerr << "解码器打开失败" << std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return nullptr;
    }
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *out_frame = nullptr;
    bool got_frame = false;
    while (av_read_frame(fmt_ctx, pkt) >= 0)
    {
        if (pkt->stream_index == stream_idx)
        {
            if (avcodec_send_packet(codec_ctx, pkt) == 0)
            {
                while (avcodec_receive_frame(codec_ctx, frame) == 0)
                {
                    got_frame = true;
                    break;
                }
            }
        }
        av_packet_unref(pkt);
        if (got_frame)
            break;
    }
    av_packet_free(&pkt);
    if (!got_frame)
    {
        std::cerr << "图片解码失败" << std::endl;
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return nullptr;
    }
    // 若不是目标格式，转换
    if (frame->format != dst_fmt)
    {
        struct SwsContext *sws_ctx = sws_getContext(
            frame->width, frame->height, (AVPixelFormat)frame->format,
            frame->width, frame->height, dst_fmt,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        out_frame = av_frame_alloc();
        out_frame->format = dst_fmt;
        out_frame->width = frame->width;
        out_frame->height = frame->height;
        av_image_alloc(out_frame->data, out_frame->linesize, frame->width, frame->height, dst_fmt, 1);
        sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, out_frame->data, out_frame->linesize);
        sws_freeContext(sws_ctx);
        av_frame_free(&frame);
    }
    else
    {
        out_frame = frame;
    }
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return out_frame;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        std::cerr << "用法: " << argv[0] << " <rknn模型路径> <图片路径> <rgb|gray>" << std::endl;
        return 1;
    }
    std::string model_path = argv[1];
    std::string img_path = argv[2];
    std::string mode = argv[3];

    RKNNModelLoader loder;
    if (!loder.load_model(model_path))
    {
        std::cerr << "模型加载失败！" << std::endl;
        return 1;
    }
    std::cout << "模型加载成功！" << std::endl;

    AVPixelFormat dst_fmt = (mode == "gray") ? AV_PIX_FMT_GRAY8 : AV_PIX_FMT_RGB24;
    AVFrame *frame = decode_image(img_path, dst_fmt);
    if (!frame)
    {
        std::cerr << "图片解码失败！" << std::endl;
        return 1;
    }
    std::cout << "图片解码成功，尺寸: " << frame->width << "x" << frame->height << std::endl;
    loder.show_model_inf();
    std::vector<float> output;
    bool ok = false;
    if (mode == "gray")
        ok = loder.infer_frame_gray(frame, output);
    else
        ok = loder.infer_frame_rgb(frame, output);

    if (ok)
    {
        std::cout << "推理成功，输出大小: " << output.size() << std::endl;
        if (!output.empty())
        {
            std::cout << "输出前10个值: ";
            for (size_t i = 0; i < std::min<size_t>(10, output.size()); ++i)
            {
                std::cout << output[i] << " ";
            }
            std::cout << std::endl;
        }
    }
    else
    {
        std::cerr << "推理失败！" << std::endl;
    }

    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    return 0;
}