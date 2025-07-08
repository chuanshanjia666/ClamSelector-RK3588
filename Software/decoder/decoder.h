#ifndef DECODER_H
#define DECODER_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixdesc.h>
#include <libavutil/log.h>
}

#include <string>

class Decoder
{
public:
    Decoder();
    ~Decoder();
    bool init(const std::string &filename, const std::string &hw_device_type);
    bool init(const std::string &filename, const std::string &hw_device_type, const std::string &codec_name);
    bool autoinit(const std::string &filename, const std::string &codec_name);
    bool decodeFrame(AVFrame **frame);
    void close();
    void printDecoderInfo();
    void printFrameInfo(AVFrame *frame);
    AVBufferRef *get_hwdevice_cxt();
    AVRational
    getTimeBase() const;

private:
    AVFormatContext *format_ctx_;
    AVCodecContext *codec_ctx_;
    AVBufferRef *hw_device_ctx_;
    int video_stream_index_;
    AVPacket *packet_;

    bool initHWDevice(const std::string &hw_device_type);
    bool initCodecContext(AVStream *stream, const AVCodec *codec);
};
#endif // DECODER_H
