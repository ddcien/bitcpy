cmake_minimum_required(VERSION 3.6)
project(bitcpy)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_EXE_LINKER_FLAGS "-static")

add_library(bitcpy_test bitcpy_test.c)

add_library(bitcpy_0 bitcpy_0.c)
add_library(bitcpy_1 bitcpy_1.c)

add_executable(bitcpy_demo_0 main.c)
target_link_libraries(bitcpy_demo_0 bitcpy_test bitcpy_0)

add_executable(bitcpy_demo_1 main.c)
target_link_libraries(bitcpy_demo_1 bitcpy_test bitcpy_1)
