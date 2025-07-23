#include "inferencethread.h"
#include <QDebug>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>

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

// 添加 softmax 函数
std::vector<float> InferenceThread::softmax(const std::vector<float> &logits)
{
    std::vector<float> probabilities(logits.size());

    // 找到最大值以提高数值稳定性
    float max_val = *std::max_element(logits.begin(), logits.end());

    // 计算 exp(x - max) 的和
    float sum = 0.0f;
    for (size_t i = 0; i < logits.size(); ++i)
    {
        probabilities[i] = std::exp(logits[i] - max_val);
        sum += probabilities[i];
    }

    // 归一化
    for (size_t i = 0; i < probabilities.size(); ++i)
    {
        probabilities[i] /= sum;
    }

    return probabilities;
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

            // 应用 softmax 处理
            std::vector<float> probabilities = softmax(output);

            // 打印原始输出和概率
            std::cout << "Raw output: ";
            for (const auto &val : output)
            {
                std::cout << val << " ";
            }
            std::cout << std::endl;

            std::cout << "Probabilities: ";
            for (const auto &prob : probabilities)
            {
                std::cout << prob << " ";
            }
            std::cout << std::endl;

            // 找到最大概率和对应类别
            auto maxIter = std::max_element(probabilities.begin(), probabilities.end());
            int classId = std::distance(probabilities.begin(), maxIter);
            float confidence = *maxIter;

            qDebug() << "[Inference] Predicted class:" << classId << ", confidence:" << confidence;

            emit inferenceResultReady(classId, confidence); // 发射处理后的结果
        }

        av_frame_free(&frame); // 释放帧内存
    }
}
