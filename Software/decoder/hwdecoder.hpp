#pragma once
#include "basedecoder.hpp"

class HWDecoder : public BaseDecoder
{
public:
    HWDecoder();
    ~HWDecoder() override;

    bool init(const std::string &filename, const std::string &hw_device_type, const std::string &codec_name);
    bool autoinit(const std::string &filename, const std::string &codec_name);
    bool decodeFrame(AVFrame **frame) override;
    void close() override;
    void printDecoderInfo() override;
    AVRational getTimeBase() const override;

    AVBufferRef *get_hwdevice_ctx();

private:
    AVFormatContext *format_ctx_;
    AVCodecContext *codec_ctx_;
    AVBufferRef *hw_device_ctx_;
    int video_stream_index_;
    AVPacket *packet_;

    bool initHWDevice(const std::string &hw_device_type);
    bool initCodecContext(AVStream *stream, const AVCodec *codec);
};