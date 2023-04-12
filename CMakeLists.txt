cmake_minimum_required(VERSION 3.12)
project(vm8086)

set(CMAKE_CXX_STANDARD 20)

include_directories(utilities/include)
add_executable(vm8086 source/main.cpp source/io.h source/io.cpp source/decoder.h source/decoder.cpp source/printer.h source/printer.cpp)