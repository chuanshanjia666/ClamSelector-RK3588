add_library(scaler STATIC
softwarescaler.cpp
softwarescaler.h
rgascaler.cpp
rgascaler.h
basescaler.cpp
basescaler.h
rgascalerplus.cpp
rgascalerplus.h
)

target_include_directories(scaler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} compile_options)


