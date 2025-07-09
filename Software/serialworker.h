#pragma once

#include <QObject>
#include <QThread>
#include <QString>
#include "serialport.hpp"

class SerialWorker : public QThread
{
    Q_OBJECT
public:
    explicit SerialWorker(const char *portName, int baudrate, QObject *parent = nullptr);
    ~SerialWorker();

    bool isOpen() const;

signals:
    void startCommandReceived();

public slots:
    void sendClassResponse(int classId);

protected:
    void run() override;

private:
    SerialPort m_serial;
};
