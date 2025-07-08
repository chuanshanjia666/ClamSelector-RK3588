option(USE_RKNN "Enable RKNN backend" ON)
option(USE_TENSORRT "Enable TensorRT backend" OFF)

set(MODELLODER_SRC
    basemodelloader.cpp
)

set(MODELLODER_LINK_LIBS compile_options)
set(MODELLODER_INCLUDE_DIR)

if(USE_RKNN)
    list(APPEND MODELLODER_SRC rknnmodelloader.cpp)
    list(APPEND MODELLODER_LINK_LIBS rknnrt)
    list(APPEND MODELLODER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

if(USE_TENSORRT)
    # list(APPEND MODELLODER_SRC tensorrtmodelloder.cpp)
    # list(APPEND MODELLODER_LINK_LIBS nvinfer)
    # list(APPEND MODELLODER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tensorrt_include)
endif()

add_library(modelloader STATIC ${MODELLODER_SRC})

target_include_directories(modelloder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MODELLODER_INCLUDE_DIR})
target_link_libraries(modelloder PUBLIC ${MODELLODER_LINK_LIBS})