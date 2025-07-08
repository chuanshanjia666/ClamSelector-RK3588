#pragma once
#include "basedecoder.hpp"

class SWDecoder : public BaseDecoder
{
public:
    SWDecoder();
    ~SWDecoder() override;

    bool init(const std::string &filename);
    bool decodeFrame(AVFrame **frame) override;
    void close() override;
    void printDecoderInfo() override;
    AVRational getTimeBase() const override;

private:
    AVFormatContext *format_ctx_;
    AVCodecContext *codec_ctx_;
    int video_stream_index_;
    AVPacket *packet_;

    bool initCodecContext(AVStream *stream, const AVCodec *codec);
};