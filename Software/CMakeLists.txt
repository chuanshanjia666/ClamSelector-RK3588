cmake_minimum_required(VERSION 3.16)

project(main VERSION 0.1 LANGUAGES CXX)
option(USE_RKNN "Enable RKNN backend" ON)
option(USE_TENSORRT "Enable TensorRT backend" OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



add_library(compile_options INTERFACE)
target_compile_definitions(compile_options INTERFACE -D__STDC_CONSTANT_MACROS)
target_compile_options(compile_options INTERFACE -O2 -Wall -Wextra -Wno-unused-parameter -g)


if(UNIX)
    # # 设置自定义FFmpeg路径
    set(FFMPEG_ROOT_DIR "/userdata/ffmpeg")
    set(FFMPEG_INCLUDE_DIR "${FFMPEG_ROOT_DIR}/include")
    set(FFMPEG_LIB_DIR "${FFMPEG_ROOT_DIR}/lib")

    # 添加FFmpeg头文件路径
    include_directories(${FFMPEG_INCLUDE_DIR})

    # 添加FFmpeg库路径
    link_directories(${FFMPEG_LIB_DIR})
    target_link_libraries(compile_options INTERFACE 
        # FFmpeg核心库（按依赖顺序排列）
        avfilter
        postproc
        swscale
        avformat
        avcodec
        avutil
        swresample
        avdevice
        
        # 系统库
        z
        m
        pthread
        dl
        
        # Rockchip特有库
        rga
        rockchip_mpp
        drm
    )
else()
    target_link_libraries(compile_options INTERFACE 
        avfilter
        postproc
        swscale
        avformat
        avcodec
        avutil
        swresample
        avdevice)

endif()

add_subdirectory(scaler)
add_subdirectory(decoder)
add_subdirectory(modelloader)
add_subdirectory(communication)
# 查找 Qt5
find_package(Qt5 REQUIRED COMPONENTS Widgets)


add_executable(main
    main.cpp
    decodethread.cpp
    decodethread.h
    mainwindow.cpp
    serialworker.cpp
    inferencethread.cpp
    inferencethread.h
    mainwindow.h
    mainwindow.ui
    resources.qrc
)



# 让主目标依赖虚拟库
target_link_libraries(main PRIVATE
    compile_options
    Qt5::Widgets
    
    
    scaler
    decoder
    modelloader
    communication
)

