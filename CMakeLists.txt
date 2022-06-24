cmake_minimum_required(VERSION 3.10)


set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
include(pico_sdk_import.cmake)

project(recorder C CXX ASM)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(TARGET_NAME "recorder")

pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

file(GLOB ${TARGET_NAME}_SRC "src/*.c")

add_subdirectory(src/no-OS-FatFS/FatFs_SPI build)

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PUBLIC src/main.c ${${TARGET_NAME}_SRC})

pico_generate_pio_header(${TARGET_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/pio/i2s_input.pio)

pico_enable_stdio_usb(${TARGET_NAME} 0)
pico_enable_stdio_uart(${TARGET_NAME} 1)

target_link_libraries(${TARGET_NAME} pico_stdlib hardware_pio FatFs_SPI)

pico_add_extra_outputs(${TARGET_NAME})

