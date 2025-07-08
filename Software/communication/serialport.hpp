#pragma once

#include <cstddef>

class SerialPort
{
public:
    SerialPort(const char *port, int baudrate);
    ~SerialPort();

    bool isOpen() const;
    int writeData(const char *data, size_t length);
    int readData(char *buffer, size_t bufferSize);

private:
    int fd;
};