#include "inferencethread.h"
#include <QDebug>
#include <chrono>


InferenceThread::InferenceThread(QObject *parent)
    : QThread(parent)
{
    if (!m_modelLoader.load_model("ResNet50v2.rknn"))
    {
        qCritical() << "[RKNN] Failed to load model!";
    }
    else
    {
        qDebug() << "[RKNN] Model loaded successfully";
    }
}

InferenceThread::~InferenceThread()
{
    stopInference();
    freeQueue();
}

void InferenceThread::freeQueue()
{
    QMutexLocker locker(&m_mutex);
    while (!m_frameQueue.isEmpty())
    {
        AVFrame *frame = m_frameQueue.dequeue();
        av_frame_free(&frame);
    }
}

void InferenceThread::enqueueFrame(AVFrame *frame)
{
    QMutexLocker locker(&m_mutex);

    // 深度复制AVFrame
    AVFrame *frameCopy = av_frame_alloc();
    av_frame_ref(frameCopy, frame);

    if (m_frameQueue.size() > 5)
    { // 限制队列长度
        AVFrame *old = m_frameQueue.dequeue();
        av_frame_free(&old);
    }
    m_frameQueue.enqueue(frameCopy);
    m_condition.wakeOne();
}

void InferenceThread::stopInference()
{
    m_running = false;
    m_condition.wakeAll();
    wait();
}

void InferenceThread::run()
{
    while (m_running)
    {
        AVFrame *frame = nullptr;
        {
            QMutexLocker locker(&m_mutex);
            while (m_frameQueue.isEmpty() && m_running)
            {
                m_condition.wait(&m_mutex);
            }
            if (!m_running)
                break;
            frame = m_frameQueue.dequeue();
        }

        // 检查帧格式
        if (frame->format != AV_PIX_FMT_RGB24 &&
            frame->format != AV_PIX_FMT_BGR24 &&
            frame->format != AV_PIX_FMT_NV12)
        {
            qWarning() << "[Inference] Unsupported pixel format:" << frame->format;
            av_frame_free(&frame);
            continue;
        }

        // 执行推理
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<float> output;
        bool success = false;

        switch (frame->format)
        {
        case AV_PIX_FMT_RGB24:
            success = m_modelLoader.infer_frame_rgb(frame, output);
            break;
        default:
            qWarning() << "Unhandled format:" << frame->format;
        }

        auto end = std::chrono::high_resolution_clock::now();

        if (success && !output.empty())
        {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            qDebug().nospace() << "[Inference] " << frame->width << "x" << frame->height
                               << " fmt:" << frame->format
                               << " time:" << duration.count() << "ms";

            // 找到最大置信度和对应类别
            auto maxIter = std::max_element(output.begin(), output.end());
            int classId = std::distance(output.begin(), maxIter);
            float confidence = *maxIter;

            qDebug() << "[Inference] Predicted class:" << classId << ", confidence:" << confidence;

            emit inferenceResultReady(classId, confidence); // 发射新信号
        }

        av_frame_free(&frame); // 释放帧内存
    }
}

