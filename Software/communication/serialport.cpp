#include "serialport.hpp"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>

SerialPort::SerialPort(const char *port, int baudrate) : fd(-1)
{
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        perror("Unable to open port");
        return;
    }

    // 配置串口参数
    struct termios options;
    tcgetattr(fd, &options);

    // 设置波特率
    cfsetispeed(&options, baudrate);
    cfsetospeed(&options, baudrate);

    // 8N1配置
    options.c_cflag &= ~PARENB; // 无奇偶校验
    options.c_cflag &= ~CSTOPB; // 1位停止位
    options.c_cflag &= ~CSIZE;  // 清除数据位掩码
    options.c_cflag |= CS8;     // 8位数据位

    options.c_cflag |= (CLOCAL | CREAD);                // 启用接收和本地模式
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 原始输入
    options.c_oflag &= ~OPOST;                          // 原始输出

    // 设置超时 - 15秒
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 150;

    tcsetattr(fd, TCSANOW, &options);
}

SerialPort::~SerialPort()
{
    if (fd != -1)
    {
        close(fd);
    }
}

bool SerialPort::isOpen() const
{
    return fd != -1;
}

int SerialPort::writeData(const char *data, size_t length)
{
    if (fd == -1)
        return -1;
    return write(fd, data, length);
}

int SerialPort::readData(char *buffer, size_t bufferSize)
{
    if (fd == -1)
        return -1;
    return read(fd, buffer, bufferSize);
}