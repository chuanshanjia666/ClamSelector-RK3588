#ifndef DECODETHREAD_H
#define DECODETHREAD_H

#include <QThread>
#include <QImage>
#include "decoder.h"
#include "basescaler.h"

extern "C"
{
#include <libavutil/frame.h>
}

class DecodeThread : public QThread
{
    Q_OBJECT
public:
    explicit DecodeThread(QObject *parent = nullptr);
    ~DecodeThread();

    void startDecode(const QString &filename);
    void stopDecode();

signals:
    // 发送解码后的AVFrame给推理线程
    void frameDecoded(AVFrame *frame);
    // 发送QImage给显示线程（可选）
    void frameDecodedForDisplay(const QImage &image);
    void decodingFinished();

protected:
    void run() override;

private:
    BaseScaler *scaler = nullptr;
    Decoder m_decoder;
    QString m_filename;
    bool m_running = false;
};

#endif // DECODETHREAD_H