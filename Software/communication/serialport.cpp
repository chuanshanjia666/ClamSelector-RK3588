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
    // 修改打开方式为阻塞模式
    fd = open(port, O_RDWR | O_NOCTTY); // 移除了O_NDELAY

    if (fd == -1)
    {
        perror("Unable to open port");
        return;
    }

    struct termios options;
    tcgetattr(fd, &options);

    // 完全重置所有选项
    cfmakeraw(&options);

    // 设置波特率
    cfsetispeed(&options, baudrate);
    cfsetospeed(&options, baudrate);

    // 8N1配置
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // 关键修复：禁用所有流控
    options.c_cflag &= ~CRTSCTS;                // 禁用硬件流控
    options.c_iflag &= ~(IXON | IXOFF | IXANY); // 禁用软件流控

    options.c_cflag |= (CLOCAL | CREAD);

    // 输入模式设置
    options.c_lflag = 0; // 非规范模式，无回显等

    // 输出模式设置
    options.c_oflag = 0; // 原始输出

    // 超时设置 - 等待最多1.5秒
    options.c_cc[VMIN] = 0;   // 最小字符数
    options.c_cc[VTIME] = 15; // 超时时间(单位是0.1秒)

    // 应用设置
    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        perror("Error setting serial attributes");
        close(fd);
        fd = -1;
        return;
    }

    // 清空缓冲区
    tcflush(fd, TCIOFLUSH);
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

    int bytes_read = read(fd, buffer, bufferSize);
    if (bytes_read < 0)
    {
        // 添加详细的错误信息
        std::cerr << "Read error: " << strerror(errno)
                  << " (errno=" << errno << ")" << std::endl;
    }
    return bytes_read;
}