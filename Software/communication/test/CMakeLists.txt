cmake_minimum_required(VERSION 3.10)
project(SerialTest)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(../)

# 源文件
set(SOURCES 
    ../serialport.cpp
    main.cpp
)

# 可执行文件
add_executable(serial_test ${SOURCES})

# 链接系统库
find_package(Threads REQUIRED)
target_link_libraries(serial_test PRIVATE Threads::Threads)

# 安装规则（可选）
install(TARGETS serial_test DESTINATION bin)