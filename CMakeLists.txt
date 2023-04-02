cmake_minimum_required(VERSION 3.12)
project(vm8086)

set(CMAKE_CXX_STANDARD 20)

add_executable(vm8086 src/main.cpp src/utils.h src/io.h src/io.cpp src/utils.cpp src/decoder.h src/decoder.cpp src/helper.h src/helper.cpp src/instructions/add.h src/instructions/sub.h src/instructions/cmp.h src/instructions/ret.h src/instructions/add.cpp src/instructions/cmp.cpp src/instructions/ret.cpp src/instructions/sub.cpp)