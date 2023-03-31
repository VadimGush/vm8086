cmake_minimum_required(VERSION 3.12)
project(vm8086)

set(CMAKE_CXX_STANDARD 20)

add_executable(vm8086 src/main.cpp src/utils.h src/io.h src/io.cpp src/utils.cpp)