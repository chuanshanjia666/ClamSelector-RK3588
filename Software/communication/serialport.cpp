#include "serialport.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <iostream>

SerialPort::SerialPort(const char *port, int baudrate) : fd(-1)
{
    // 打开串口设备
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        perror("无法打开串口");
        return;
    }

    printf("串口 %s 打开成功\n", port);

    struct termios options;

    // 获取当前串口配置
    if (tcgetattr(fd, &options) != 0)
    {
        perror("获取串口配置失败");
        close(fd);
        fd = -1;
        return;
    }

    // 设置波特率
    speed_t speed;
    switch (baudrate)
    {
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    case 230400:
        speed = B230400;
        break;
    case 460800:
        speed = B460800;
        break;
    case 921600:
        speed = B921600;
        break;
    default:
        speed = B115200; // 默认115200
        printf("不支持的波特率 %d，使用默认值 115200\n", baudrate);
        break;
    }

    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

    // 设置数据位、停止位、校验位 (8N1)
    options.c_cflag &= ~PARENB; // 无校验位
    options.c_cflag &= ~CSTOPB; // 1个停止位
    options.c_cflag &= ~CSIZE;  // 清除数据位设置
    options.c_cflag |= CS8;     // 8个数据位

    // 设置为原始模式
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    // 设置超时时间
    options.c_cc[VMIN] = 0;   // 最小字符数
    options.c_cc[VTIME] = 10; // 超时时间(单位是0.1秒)

    // 应用配置
    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        perror("设置串口参数失败");
        close(fd);
        fd = -1;
        return;
    }

    printf("串口配置成功：%d 8N1\n", baudrate);

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

    // 发送数据
    int bytes_written = write(fd, data, length);
    if (bytes_written < 0)
    {
        perror("发送数据失败");
        return -1;
    }

    printf("成功发送 %d 字节\n", bytes_written);

    // 刷新输出缓冲区，确保数据发送完成
    tcdrain(fd);

    return bytes_written;
}

int SerialPort::readData(char *buffer, size_t bufferSize)
{
    if (fd == -1)
        return -1;

    int bytes_read = read(fd, buffer, bufferSize);
    if (bytes_read < 0)
    {
        // 检查是否是超时错误
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 这是正常的超时，不是错误
            return 0;
        }
        // 其他错误
        perror("读取数据失败");
        return -1;
    }
    else if (bytes_read > 0)
    {
        printf("成功读取 %d 字节\n", bytes_read);
    }

    return bytes_read;
}