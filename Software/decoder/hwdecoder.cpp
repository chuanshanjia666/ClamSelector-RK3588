#include "hwdecoder.hpp"
#include <iostream>

extern "C"
{
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
}

HWDecoder::HWDecoder()
    : format_ctx_(nullptr), codec_ctx_(nullptr), hw_device_ctx_(nullptr),
      video_stream_index_(-1), packet_(nullptr)
{
    packet_ = av_packet_alloc();
}

HWDecoder::~HWDecoder()
{
    close();
}

bool HWDecoder::init(const std::string &filename, const std::string &hw_device_type, const std::string &codec_name)
{
    if (avformat_open_input(&format_ctx_, filename.c_str(), nullptr, nullptr) != 0)
    {
        std::cerr << "Could not open input file: " << filename << std::endl;
        return false;
    }
    if (avformat_find_stream_info(format_ctx_, nullptr) < 0)
    {
        std::cerr << "Could not find stream information" << std::endl;
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
        std::cerr << "Could not find video stream" << std::endl;
        close();
        return false;
    }
    AVStream *video_stream = format_ctx_->streams[video_stream_index_];
    const AVCodec *codec = avcodec_find_decoder_by_name(codec_name.c_str());
    if (!codec)
    {
        std::cerr << "Decoder '" << codec_name << "' not found!" << std::endl;
        close();
        return false;
    }
    if (codec->id != video_stream->codecpar->codec_id)
    {
        std::cerr << "Warning: Specified decoder '" << codec_name << "' does not match stream codec id!" << std::endl;
    }
    if (!initHWDevice(hw_device_type))
    {
        std::cerr << "Failed to initialize hardware device" << std::endl;
        close();
        return false;
    }
    if (!initCodecContext(video_stream, codec))
    {
        std::cerr << "Failed to initialize codec context" << std::endl;
        close();
        return false;
    }
    return true;
}

bool HWDecoder::initHWDevice(const std::string &hw_device_type)
{
    enum AVHWDeviceType type = av_hwdevice_find_type_by_name(hw_device_type.c_str());
    if (type == AV_HWDEVICE_TYPE_NONE)
        return false;
    int ret = av_hwdevice_ctx_create(&hw_device_ctx_, type, nullptr, nullptr, 0);
    if (ret < 0)
    {
        hw_device_ctx_ = nullptr;
        return false;
    }
    return true;
}

bool HWDecoder::autoinit(const std::string &filename, const std::string &codec_name)
{
    //TO DO
}

bool HWDecoder::initCodecContext(AVStream *stream, const AVCodec *codec)
{
    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_)
        return false;
    if (avcodec_parameters_to_context(codec_ctx_, stream->codecpar) < 0)
        return false;
    codec_ctx_->hw_device_ctx = av_buffer_ref(hw_device_ctx_);
    if (!codec_ctx_->hw_device_ctx)
        return false;
    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0)
        return false;
    return true;
}

bool HWDecoder::decodeFrame(AVFrame **frame)
{
    int ret;
    if (!frame)
        return false;
    if (*frame)
        av_frame_free(frame); // 保证frame为nullptr
    while (true)
    {
        ret = av_read_frame(format_ctx_, packet_);
        if (ret < 0)
        {
            // 文件结束或错误
            return false;
        }
        if (packet_->stream_index == video_stream_index_)
        {
            ret = avcodec_send_packet(codec_ctx_, packet_);
            av_packet_unref(packet_);
            if (ret < 0)
            {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                std::cerr << "Error sending packet to decoder: " << av_make_error_string(errbuf, sizeof(errbuf), ret) << std::endl;
                continue;
            }
            *frame = av_frame_alloc();
            ret = avcodec_receive_frame(codec_ctx_, *frame);
            if (ret == 0)
            {
                return true;
            }
            else if (ret == AVERROR(EAGAIN))
            {
                av_frame_free(frame);
                continue;
            }
            else
            {
                char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
                std::cerr << "Error receiving frame: " << av_make_error_string(errbuf, sizeof(errbuf), ret) << std::endl;
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

void HWDecoder::close()
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
    if (hw_device_ctx_)
    {
        av_buffer_unref(&hw_device_ctx_);
        hw_device_ctx_ = nullptr;
    }
    if (packet_)
    {
        av_packet_unref(packet_);
        av_packet_free(&packet_);
        packet_ = nullptr;
    }
    video_stream_index_ = -1;
}

void HWDecoder::printDecoderInfo()
{
    if (!codec_ctx_)
    {
        std::cerr << "Codec context not initialized!" << std::endl;
        return;
    }

    std::cout << "\n======= Decoder Info =======" << std::endl;
    std::cout << "Decoder name: " << codec_ctx_->codec->name << std::endl;
    std::cout << "Hardware acceleration: " << (codec_ctx_->hw_device_ctx ? "Yes" : "No") << std::endl;

    if (codec_ctx_->hw_device_ctx)
    {
        AVHWDeviceContext *hw_device_ctx = (AVHWDeviceContext *)codec_ctx_->hw_device_ctx->data;
        std::cout << "HW device type: " << av_hwdevice_get_type_name(hw_device_ctx->type) << std::endl;
    }

    // 打印当前像素格式
    std::cout << "Current pixel format: " << av_get_pix_fmt_name(codec_ctx_->pix_fmt) << std::endl;

    // 打印支持的硬件像素格式（推荐方式）
    std::cout << "Supported hardware pixel formats: ";
    const AVCodecHWConfig *config = nullptr;
    int i = 0;
    while ((config = avcodec_get_hw_config(codec_ctx_->codec, i++)))
    {
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
        {
            std::cout << av_get_pix_fmt_name(config->pix_fmt) << " ";
        }
    }
    std::cout << std::endl;
    std::cout << "===========================\n"
              << std::endl;
}


AVRational HWDecoder::getTimeBase() const
{
    return format_ctx_ ? format_ctx_->streams[video_stream_index_]->time_base : AVRational{1, 1};
}

AVBufferRef *HWDecoder::get_hwdevice_ctx()
{
    return hw_device_ctx_;
}