#include "decoder.h"
#include <iostream>

Decoder::Decoder() : format_ctx_(nullptr),
                     codec_ctx_(nullptr),
                     hw_device_ctx_(nullptr),
                     video_stream_index_(-1),
                     packet_(nullptr)
{
    packet_ = av_packet_alloc();
}

Decoder::~Decoder()
{
    close();
}

AVRational Decoder::getTimeBase() const
{
    return format_ctx_->streams[video_stream_index_]->time_base;
}

bool Decoder::init(const std::string &filename, const std::string &hw_device_type, const std::string &codec_name)
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

bool Decoder::init(const std::string &filename, const std::string &hw_device_type)
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
    // 自动选择解码器
    const AVCodec *codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (!codec)
    {
        std::cerr << "Unsupported codec" << std::endl;
        close();
        return false;
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

// 新增自动硬件类型优先级尝试的autoinit
bool Decoder::autoinit(const std::string &filename, const std::string &codec_name)
{
    close();
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
    // 自动尝试硬件类型
    const char *hw_priority[] = {"dxva2", "d3d11va", nullptr};
    for (int i = 0; hw_priority[i]; i++)
    {
        if (initHWDevice(hw_priority[i]))
        {
            if (initCodecContext(video_stream, codec))
                return true;
            else
                close();
        }
    }
    std::cerr << "Failed to initialize any hardware device for autoinit." << std::endl;
    close();
    return false;
}

bool Decoder::initHWDevice(const std::string &hw_device_type)
{
    enum AVHWDeviceType type = av_hwdevice_find_type_by_name(hw_device_type.c_str());
    if (type == AV_HWDEVICE_TYPE_NONE)
    {
        std::cerr << "No valid HW acceleration type found: " << hw_device_type << std::endl;
        return false;
    }
    int ret = av_hwdevice_ctx_create(&hw_device_ctx_, type, nullptr, nullptr, 0);
    if (ret < 0)
    {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Failed to create HW context for "
                  << av_hwdevice_get_type_name(type)
                  << ": " << errbuf << std::endl;
        hw_device_ctx_ = nullptr;
        return false;
    }
    std::cout << "Using HW acceleration: " << av_hwdevice_get_type_name(type) << std::endl;
    return true;
}

bool Decoder::initCodecContext(AVStream *stream, const AVCodec *codec)
{
    // 创建解码器上下文
    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_)
    {
        std::cerr << "Failed to allocate codec context" << std::endl;
        return false;
    }

    // 将流参数复制到解码器上下文
    if (avcodec_parameters_to_context(codec_ctx_, stream->codecpar) < 0)
    {
        std::cerr << "Failed to copy codec parameters to context" << std::endl;
        return false;
    }

    // 设置硬件设备上下文
    codec_ctx_->hw_device_ctx = av_buffer_ref(hw_device_ctx_);
    if (!codec_ctx_->hw_device_ctx)
    {
        std::cerr << "Failed to reference hardware device context" << std::endl;
        return false;
    }

    // 打开解码器
    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0)
    {
        std::cerr << "Failed to open codec" << std::endl;
        return false;
    }

    return true;
}

bool Decoder::decodeFrame(AVFrame **frame)
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

void Decoder::close()
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

void Decoder::printDecoderInfo()
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

void Decoder::printFrameInfo(AVFrame *frame)
{
    if (!frame)
    {
        std::cerr << "Frame is null!" << std::endl;
        return;
    }

    std::cout << "\n======= Frame Info =======" << std::endl;
    std::cout << "Decode type: " << (frame->hw_frames_ctx ? "Hardware" : "Software") << std::endl;

    if (frame->hw_frames_ctx)
    {
        AVHWFramesContext *hw_frames_ctx = (AVHWFramesContext *)frame->hw_frames_ctx->data;
        std::cout << "HW frame storage type: " << av_hwdevice_get_type_name(hw_frames_ctx->device_ctx->type) << std::endl;
    }

    std::cout << "Pixel format: " << av_get_pix_fmt_name((AVPixelFormat)frame->format) << std::endl;
    std::cout << "Resolution: " << frame->width << "x" << frame->height << std::endl;
    std::cout << "Frame type: " << av_get_picture_type_char(frame->pict_type) << std::endl;
    std::cout << "PTS: " << frame->pts << std::endl;
    std::cout << "===========================" << std::endl;
}

AVBufferRef *Decoder::get_hwdevice_cxt()
{
    return hw_device_ctx_;
}