#pragma once
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/log.h>
#include <libavutil/mem.h>
}

#include "rknnmodelloader.h"

class InferenceThread : public QThread
{
    Q_OBJECT
public:
    explicit InferenceThread(QObject *parent = nullptr);
    ~InferenceThread();

    void enqueueFrame(AVFrame *frame); // 改为接收AVFrame
    void stopInference();
signals:
    void inferenceResultReady(int classId, float confidence);

protected:
    void run() override;

private:
    QMutex m_mutex;
    QWaitCondition m_condition;
    QQueue<AVFrame *> m_frameQueue; // 存储AVFrame指针
    bool m_running = true;
    RKNNModelLoader m_modelLoader;

    void freeQueue(); // 清理队列中的AVFrame
};