#ifndef DECODETHREAD_H
#define DECODETHREAD_H

#include <QThread>
#include <QImage>
#include "decoder.h"
#include "basescaler.h"
class DecodeThread : public QThread
{
    Q_OBJECT
public:
    explicit DecodeThread(QObject *parent = nullptr);
    ~DecodeThread();

    void startDecode(const QString &filename);
    void stopDecode();

signals:
    void frameDecoded(const QImage &image);
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
