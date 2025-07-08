cmake_minimum_required(VERSION 3.16)

project(main VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



add_library(compile_options INTERFACE)
target_compile_definitions(compile_options INTERFACE -D__STDC_CONSTANT_MACROS)
target_compile_options(compile_options INTERFACE -O2 -Wall -Wextra -Wno-unused-parameter -g)


target_link_directories(compile_options INTERFACE
    /userdata/ffmpeg/lib
)
target_link_libraries(compile_options INTERFACE 
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
    drm)




add_executable(main
    main.cpp
    ../basemodelloader.cpp
    ../rknnmodelloader.cpp
)


target_include_directories(main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../include)
# 让主目标依赖虚拟库
target_link_libraries(main PRIVATE
    compile_options
    rknnrt
)

