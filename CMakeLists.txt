cmake_minimum_required(VERSION 3.13)
project(SimpleRTOS C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

add_library(MRtos_lib)
add_subdirectory(src/Kernel)

add_definitions(-DTEST)