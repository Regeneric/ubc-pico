cmake_minimum_required(VERSION 3.12)

set(PICO_SDK_PATH /usr/share/pico-sdk)
set(PICO_EXTRAS_PATH /home/hkk/Documents/c/pico/pico-extras)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico)

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(Pico-UBC C CXX ASM)

pico_sdk_init()

add_compile_options(
    -Wno-format                 # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
    -Wno-unused-function        # we have some for the docs that aren't called
)

add_subdirectory(src)