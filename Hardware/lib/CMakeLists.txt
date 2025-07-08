
add_library(my_driver STATIC
    src/dc_motor.c
)

target_include_directories(my_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)
