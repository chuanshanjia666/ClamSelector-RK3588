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

// void DecodeThread::run()
// {
//     if (!m_decoder.init("test.mp4", "rkmpp", "h264_rkmpp"))
//     {
//         qWarning() << "Failed to initialize decoder";
//         emit decodingFinished();
//         return;
//     }

//     m_decoder.printDecoderInfo();

//     int last_width = 0, last_height = 0, last_src_fmt = -1;
//     AVFrame *rgbFrame = nullptr;
//     AVFrame *frame = nullptr;
//     while (m_running)
//     {
//         if (!m_decoder.decodeFrame(&frame))
//         {
//             qDebug() << "Decoding finished or error occurred";
//             break;
//         }

//         m_decoder.printFrameInfo(frame);

//         QImage image;
//         AVFrame *inputFrame = frame;
//         AVFrame *swFrame = nullptr;

//         if (swFrame)
//             av_frame_free(&swFrame);
//         swFrame = av_frame_alloc();

//         if (av_hwframe_transfer_data(swFrame, inputFrame, 0) < 0)
//         {
//             qWarning() << "Failed to transfer CUDA frame to software frame";
//             av_frame_free(&swFrame);
//             swFrame = nullptr;
//         }
//         else
//         {
//             inputFrame = swFrame;
//         }
//         if (!scaler || inputFrame->width != last_width || inputFrame->height != last_height || inputFrame->format != last_src_fmt)
//         {
//             if (scaler)
//                 delete scaler;
//             qDebug() << "准备创建GRAScaler, width:" << inputFrame->width << "height:" << inputFrame->height << "format:" << inputFrame->format;
//             scaler = new RGAScaler(inputFrame->width, inputFrame->height, (AVPixelFormat)inputFrame->format,
//                                    m_decoder.getTimeBase(), 640, 360, AV_PIX_FMT_RGB24);
//             if (!scaler->Init())
//             {
//                 qWarning() << "GRAScaler Init failed!";
//                 delete scaler;
//                 scaler = nullptr;
//             }
//             else
//             {
//                 qDebug() << "GRAScaler Init success";
//             }

//             last_width = inputFrame->width;
//             last_height = inputFrame->height;
//             last_src_fmt = inputFrame->format;
//         }
//         if (scaler)
//         {
//             if (rgbFrame)
//             {
//                 av_frame_free(&rgbFrame);
//                 rgbFrame = nullptr;
//             }
//             qDebug() << "调用scale_and_convert";
//             rgbFrame = scaler->scale_and_convert(inputFrame);
//             if (rgbFrame && rgbFrame->format == AV_PIX_FMT_RGB24)
//             {
//                 qDebug() << "scale_and_convert成功, 输出尺寸:" << rgbFrame->width << "x" << rgbFrame->height;
//                 image = QImage(rgbFrame->data[0], rgbFrame->width, rgbFrame->height, rgbFrame->linesize[0], QImage::Format_RGB888).copy();
//             }
//             else
//             {
//                 qWarning() << "VideoScaler failed or output format not RGB24";
//             }
//         }
//         else
//         {
//             qWarning() << "VideoScaler alloc failed";
//         }

//         // 发送帧到主线程显示
//         emit frameDecoded(image);
//         if (swFrame)
//             av_frame_free(&swFrame);
//         av_frame_free(&frame);
//         QThread::msleep(33); // 约30fps
//     }

//     if (scaler)
//         delete scaler;
//     if (rgbFrame)
//         av_frame_free(&rgbFrame);

//     m_decoder.close();
//     emit decodingFinished();
// }

// void DecodeThread::run()
// {
//     // 1. 初始化硬件解码器
//     if (!m_decoder.init("test.mp4", "rkmpp", "h264_rkmpp"))
//     {
//         qWarning() << "Failed to initialize decoder";
//         emit decodingFinished();
//         return;
//     }


//     m_decoder.printDecoderInfo();

//     int last_width = 0, last_height = 0;
//     AVFrame *rgbFrame = nullptr;
//     AVFrame *frame = nullptr;
//     AVFrame *sw_frame = nullptr; // 用于硬件->软件转换的帧

//     while (m_running)
//     {
//         // 3. 解码帧
//         if (!m_decoder.decodeFrame(&frame))
//         {
//             qDebug() << "Decoding finished or error occurred";
//             break;
//         }

//         m_decoder.printFrameInfo(frame);

//         QImage image;
//         AVFrame *inputFrame = frame;

//         // 4. 从硬件下载帧到系统内存（如果需要）
//         if (inputFrame->format == AV_PIX_FMT_DRM_PRIME)
//         {
//             sw_frame = av_frame_alloc();
//             if (av_hwframe_transfer_data(sw_frame, inputFrame, 0) < 0)
//             {
//                 qWarning() << "Failed to transfer hardware frame to software";
//                 av_frame_free(&sw_frame);
//                 continue;
//             }
//             inputFrame = sw_frame;
//         }

//         // 5. 初始化或更新缩放器
//         if (!scaler || inputFrame->width != last_width || inputFrame->height != last_height)
//         {
//             delete scaler;
//             qDebug() << "Creating RGAScaler, width:" << inputFrame->width
//                      << "height:" << inputFrame->height
//                      << "format:" << av_get_pix_fmt_name((AVPixelFormat)inputFrame->format);

//             // 使用共享的硬件上下文
//             scaler = new RGAScaler(inputFrame->width, inputFrame->height,
//                                    (AVPixelFormat)inputFrame->format,
//                                    640, 360, AV_PIX_FMT_RGB24,
//                                    m_decoder.get_hwdevice_cxt());

//             if (!scaler->Init())
//             {
//                 qWarning() << "RGAScaler Init failed!";
//                 delete scaler;
//                 scaler = nullptr;
//             }
//             else
//             {
//                 qDebug() << "RGAScaler Init success";
//                 last_width = inputFrame->width;
//                 last_height = inputFrame->height;
//             }
//         }

//         // 6. 执行缩放和格式转换
//         if (scaler)
//         {
//             av_frame_free(&rgbFrame);
//             rgbFrame = scaler->scale_and_convert(inputFrame);

//             if (rgbFrame && rgbFrame->format == AV_PIX_FMT_RGB24)
//             {
//                 image = QImage(rgbFrame->data[0], rgbFrame->width, rgbFrame->height,
//                                rgbFrame->linesize[0], QImage::Format_RGB888)
//                             .copy();
//                 emit frameDecoded(image);
//             }
//             else
//             {
//                 qWarning() << "Scale/convert failed";
//             }
//         }

//         // 7. 释放资源
//         if (sw_frame)
//         {
//             av_frame_free(&sw_frame);
//             sw_frame = nullptr;
//         }
//         av_frame_free(&frame);
//         QThread::msleep(33); // 约30fps
//     }

//     // 8. 清理资源
//     delete scaler;
//     av_frame_free(&rgbFrame);
//     m_decoder.close();
//     emit decodingFinished();
// }

void DecodeThread::run()
{
    // 1. 初始化硬件解码器
    if (!m_decoder.init("test.mp4", "rkmpp", "h264_rkmpp"))
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
    AVFrame *out_frame = nullptr;

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

            qDebug() << "Creating RGAScalerPlus for" << frame->width << "x" << frame->height;
            scaler = new RGAScalerPlus(640, 360, AV_PIX_FMT_RGB24, hw_ctx);

            if (!scaler->Init())
            {
                qWarning() << "RGAScalerPlus init failed";
                delete scaler;
                scaler = nullptr;
                av_frame_free(&frame);
                break;
            }

            last_width = frame->width;
            last_height = frame->height;
        }

        // 6. 处理帧
        if (scaler)
        {
            // 释放前一帧输出
            if (out_frame)
            {
                av_frame_free(&out_frame);
            }

            out_frame = scaler->scale_and_convert(frame);
            if (out_frame && out_frame->format == AV_PIX_FMT_RGB24)
            {
                QImage img(out_frame->data[0],
                           out_frame->width,
                           out_frame->height,
                           out_frame->linesize[0],
                           QImage::Format_RGB888);
                emit frameDecoded(img.copy());
            }
            else
            {
                qWarning() << "Scale/convert failed";
            }
        }

        // 7. 释放资源
        av_frame_free(&frame);
        QThread::msleep(33); // 控制帧率
    }

    // 8. 清理资源
    if (scaler)
        delete scaler;
    if (out_frame)
        av_frame_free(&out_frame);
    m_decoder.close();
    emit decodingFinished();
}