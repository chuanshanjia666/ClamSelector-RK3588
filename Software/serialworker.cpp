#include "serialworker.h"
#include <QDebug>

SerialWorker::SerialWorker(const char *portName, int baudrate, QObject *parent)
    : QThread(parent), m_serial(portName, baudrate)
{
}

SerialWorker::~SerialWorker()
{
    requestInterruption(); // 安全中断线程
    wait();                // 等待线程安全退出
}

bool SerialWorker::isOpen() const
{
    return m_serial.isOpen();
}

void SerialWorker::sendClassResponse(int classId)
{
    if (!m_serial.isOpen())
        return;

    QString msg = QString("class%1\n").arg(classId);
    QByteArray ascii = msg.toLatin1(); // 保证是 ASCII 格式
    m_serial.writeData(ascii.constData(), ascii.length());
}

void SerialWorker::run()
{
    char buffer[256];
    QByteArray recvBuffer;

    while (!isInterruptionRequested() && m_serial.isOpen())
    {
        int n = m_serial.readData(buffer, sizeof(buffer) - 1);
        if (n > 0)
        {
            buffer[n] = '\0';
            recvBuffer.append(buffer, n);

            if (recvBuffer.contains("start\n"))
            {
                emit startCommandReceived();
                recvBuffer.clear();
            }
        }

        msleep(10);
    }
}
