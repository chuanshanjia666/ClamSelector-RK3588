#include "decodethread.h"
#include <QDebug>

extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libavutil/hwcontext_drm.h>
}

#include "rgascaler.h"
#include "softwarescaler.h"
#include "rgascalerplus.h"

DecodeThread::DecodeThread(QObject *parent)
    : QThread(parent)
{
}

DecodeThread::~DecodeThread()
{
    stopDecode();
}

void DecodeThread::startDecode(const QString &filename)
{
    m_filename = filename;
    m_running = true;
    start();
}

void DecodeThread::stopDecode()
{
    m_running = false;
    wait();
}

void DecodeThread::run()
{
    // 1. 初始化硬件解码器
    if (!m_decoder.init(m_filename.toStdString(), "rkmpp", "h264_rkmpp"))
    {
        qWarning() << "Failed to initialize decoder";
        emit decodingFinished();
        return;
    }

    m_decoder.printDecoderInfo();

    // 2. 获取硬件上下文
    AVBufferRef *hw_ctx = m_decoder.get_hwdevice_cxt();
    if (!hw_ctx)
    {
        qCritical() << "No valid hardware context";
        m_decoder.close();
        emit decodingFinished();
        return;
    }

    // 3. 初始化变量
    RGAScalerPlus *scaler = nullptr;
    int last_width = 0, last_height = 0;
    AVFrame *frame = nullptr;
    AVFrame *rgb_frame = nullptr;
    AVFrame *infer_frame = nullptr;
    BaseScaler *sclaer2 = nullptr;
    while (m_running)
    {
        // 4. 解码帧
        if (!m_decoder.decodeFrame(&frame))
        {
            qDebug() << "Decoding finished";
            break;
        }

        // 5. 创建/更新缩放器
        if (!scaler || frame->width != last_width || frame->height != last_height)
        {
            // 释放旧缩放器
            if (scaler)
            {
                delete scaler;
                scaler = nullptr;
            }

            // qDebug() << "Creating RGAScalerPlus for" << frame->width << "x" << frame->height;
            scaler = new RGAScalerPlus(640, 360, AV_PIX_FMT_RGB24, hw_ctx);
            sclaer2 = new SoftwareScaler(640, 360, AV_PIX_FMT_RGB24, {1, 1}, 384, 216, AV_PIX_FMT_RGB24);
            if (!scaler->Init())
            {
                qWarning() << "RGAScalerPlus init failed";
                delete scaler;
                scaler = nullptr;
                av_frame_free(&frame);
                break;
            }
            if (!sclaer2->Init())
            {
                qWarning() << "RGAScaler init failed";
                delete scaler;
                scaler = nullptr;
                av_frame_free(&frame);
                break;
            }
            std::cout << "init scaler success" << std::endl;
            last_width = frame->width;
            last_height = frame->height;
        }

        // 6. 处理帧
        if (scaler)
        {
            // 释放前一帧输出
            if (rgb_frame)
            {
                av_frame_free(&rgb_frame);
                rgb_frame = nullptr;
            }
            if (infer_frame)
            {
                av_frame_free(&infer_frame);
                infer_frame = nullptr;
            }

            rgb_frame = scaler->scale_and_convert(frame);
            if (rgb_frame && rgb_frame->format == AV_PIX_FMT_RGB24)
            {
                // 发送RGB帧给推理线程
                // 同时发送给显示线程（如果需要）
                QImage img(rgb_frame->data[0],
                           rgb_frame->width,
                           rgb_frame->height,
                           rgb_frame->linesize[0],
                           QImage::Format_RGB888);
                emit frameDecodedForDisplay(img.copy());
            }
            else
            {
                qWarning() << "Scale/convert failed";
            }


            infer_frame = sclaer2->scale_and_convert(rgb_frame);
            emit frameDecoded(infer_frame);
        }

        // 7. 释放资源
        av_frame_free(&frame);
        QThread::msleep(33); // 控制帧率
    }

    // 8. 清理资源
    if (scaler)
        delete scaler;
    if (rgb_frame)
        av_frame_free(&rgb_frame);
    m_decoder.close();
    emit decodingFinished();
}