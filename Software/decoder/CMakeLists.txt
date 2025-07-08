# add_library(decoder STATIC
# decoder.cpp
# decoder.h

# )
add_library(decoder STATIC
    basedecoder.cpp
    swdecoder.cpp
    hwdecoder.cpp

)


target_include_directories(decoder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} compile_options)


