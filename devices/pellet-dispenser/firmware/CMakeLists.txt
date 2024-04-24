cmake_minimum_required(VERSION 3.17)

set(PICO_SDK_FETCH_FROM_GIT On)
set(PICO_SDK_WANTED_VERSION 1.5.1)

include(pico_sdk_import.cmake)

project(pellet-dispenser)

include(ExternalProject)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(OpenOCD)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

set(FAMILY rp2040)
set(BOARD pico_sdk)
pico_sdk_init()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED 1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 1)

add_compile_options("-Wno-c99-designator")

add_subdirectory(src)
