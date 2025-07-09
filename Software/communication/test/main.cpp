#include "serialport.hpp"
#include <iostream>
#include <string>
#include <unistd.h> // for usleep

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <serial_port> [baudrate]" << std::endl;
        std::cerr << "Example: " << argv[0] << " /dev/ttyS1 115200" << std::endl;
        return 1;
    }

    const char *port = argv[1];
    int baudrate = (argc > 2) ? atoi(argv[2]) : 115200;


    // 创建串口对象
    SerialPort serial(port, baudrate);

    if (!serial.isOpen())
    {
        std::cerr << "Failed to open serial port: " << port << std::endl;
        return 1;
    }

    std::cout << "Serial port " << port << " opened successfully at " << baudrate << " baud" << std::endl;

    // 测试数据
    const char *test_data = "Hello, RK3588 Serial Port!\n";
    const int test_length = strlen(test_data);

    // 发送测试数据
    std::cout << "Sending data: " << test_data;
    int bytes_sent = serial.writeData(test_data, test_length);

    if (bytes_sent < 0)
    {
        std::cerr << "Error writing to serial port" << std::endl;
    }
    else
    {
        std::cout << "Sent " << bytes_sent << " bytes" << std::endl;
    }

    // 接收数据
    char buffer[256];
    int total_received = 0;
    int timeout_ms = 1000; // 1秒超时
    int elapsed_ms = 0;
    const int poll_interval_ms = 10;

    std::cout << "Waiting for response..." << std::endl;

    while (elapsed_ms < timeout_ms)
    {
        int bytes_read = serial.readData(buffer + total_received, sizeof(buffer) - total_received - 1);

        if (bytes_read > 0)
        {
            total_received += bytes_read;
            buffer[total_received] = '\0'; // 确保字符串终止

            // 检查是否收到完整数据（例如以换行符结尾）
            if (buffer[total_received - 1] == '\n')
            {
                break;
            }
        }
        else if (bytes_read < 0)
        {
            std::cerr << "Error reading from serial port" << std::endl;
            break;
        }

        usleep(poll_interval_ms * 1000);
        elapsed_ms += poll_interval_ms;
    }

    if (total_received > 0)
    {
        std::cout << "Received " << total_received << " bytes: " << buffer;
    }
    else
    {
        std::cout << "No data received within timeout period" << std::endl;
    }

    return 0;
}