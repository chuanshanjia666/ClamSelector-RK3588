#pragma once
#include <string>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/buffer.h>
}

class BaseDecoder
{
public:
    virtual ~BaseDecoder() = default;

    virtual bool decodeFrame(AVFrame **frame) = 0;
    virtual void close() = 0;
    virtual void printDecoderInfo() = 0;
    static void printFrameInfo(AVFrame *frame);
    virtual AVRational getTimeBase() const = 0;
};