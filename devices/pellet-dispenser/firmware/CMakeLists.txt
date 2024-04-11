cmake_minimum_required(VERSION 3.17)

set(PICO_SDK_FETCH_FROM_GIT On)
set(PICO_SDK_WANTED_VERSION 1.5.1)

include(pico_sdk_import.cmake)

project(pellet-dispenser)

include(ExternalProject)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(OpenOCD)

pico_sdk_init()

add_executable(pellet-dispenser src/main.c)

target_link_libraries(pellet-dispenser pico_stdlib)

pico_add_extra_outputs(pellet-dispenser)
add_openocd_upload_target(TARGET pellet-dispenser)
