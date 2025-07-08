#include "swdecoder.hpp"
#include <iostream>
extern "C"
{
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
}
SWDecoder::SWDecoder()
    : format_ctx_(nullptr), codec_ctx_(nullptr), video_stream_index_(-1), packet_(nullptr)
{
    packet_ = av_packet_alloc();
}

SWDecoder::~SWDecoder()
{
    close();
}

bool SWDecoder::init(const std::string &filename)
{
    close();
    if (avformat_open_input(&format_ctx_, filename.c_str(), nullptr, nullptr) != 0)
        return false;
    if (avformat_find_stream_info(format_ctx_, nullptr) < 0)
    {
        close();
        return false;
    }
    video_stream_index_ = -1;
    for (unsigned int i = 0; i < format_ctx_->nb_streams; i++)
    {
        if (format_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_index_ = i;
            break;
        }
    }
    if (video_stream_index_ == -1)
    {
        close();
        return false;
    }
    AVStream *video_stream = format_ctx_->streams[video_stream_index_];
    const AVCodec *codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (!codec)
    {
        close();
        return false;
    }
    if (!initCodecContext(video_stream, codec))
    {
        close();
        return false;
    }
    return true;
}

bool SWDecoder::initCodecContext(AVStream *stream, const AVCodec *codec)
{
    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_)
        return false;
    if (avcodec_parameters_to_context(codec_ctx_, stream->codecpar) < 0)
        return false;
    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0)
        return false;
    return true;
}

bool SWDecoder::decodeFrame(AVFrame **frame)
{
    int ret;
    if (!frame)
        return false;
    if (*frame)
        av_frame_free(frame);
    while (true)
    {
        ret = av_read_frame(format_ctx_, packet_);
        if (ret < 0)
            return false;
        if (packet_->stream_index == video_stream_index_)
        {
            ret = avcodec_send_packet(codec_ctx_, packet_);
            av_packet_unref(packet_);
            if (ret < 0)
                continue;
            *frame = av_frame_alloc();
            ret = avcodec_receive_frame(codec_ctx_, *frame);
            if (ret == 0)
                return true;
            else if (ret == AVERROR(EAGAIN))
            {
                av_frame_free(frame);
                continue;
            }
            else
            {
                av_frame_free(frame);
                return false;
            }
        }
        else
        {
            av_packet_unref(packet_);
        }
    }
}

void SWDecoder::close()
{
    if (codec_ctx_)
    {
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
    }
    if (format_ctx_)
    {
        avformat_close_input(&format_ctx_);
        format_ctx_ = nullptr;
    }
    if (packet_)
    {
        av_packet_unref(packet_);
        av_packet_free(&packet_);
        packet_ = nullptr;
    }
    video_stream_index_ = -1;
}

void SWDecoder::printDecoderInfo()
{
    if (!format_ctx_ || video_stream_index_ < 0)
    {
        std::cout << "[SWDecoder] Decoder not initialized." << std::endl;
        return;
    }
    AVStream *stream = format_ctx_->streams[video_stream_index_];
    AVCodecParameters *codecpar = stream->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);

    std::cout << "========== SWDecoder Info ==========" << std::endl;
    std::cout << "File: " << (format_ctx_->url ? format_ctx_->url : "(unknown)") << std::endl;
    std::cout << "Codec: " << (codec ? codec->long_name : "unknown") << std::endl;
    std::cout << "Resolution: " << codecpar->width << "x" << codecpar->height << std::endl;
    std::cout << "Pixel Format: " << av_get_pix_fmt_name(static_cast<AVPixelFormat>(codecpar->format)) << std::endl;
    std::cout << "Frame Rate: ";
    if (stream->avg_frame_rate.den && stream->avg_frame_rate.num)
        std::cout << av_q2d(stream->avg_frame_rate);
    else
        std::cout << "unknown";
    std::cout << std::endl;
    std::cout << "Time Base: " << stream->time_base.num << "/" << stream->time_base.den << std::endl;
    std::cout << "====================================" << std::endl;
}

AVRational SWDecoder::getTimeBase() const
{
    return format_ctx_ ? format_ctx_->streams[video_stream_index_]->time_base : AVRational{1, 1};
}