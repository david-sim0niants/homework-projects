cmake_minimum_required(VERSION 3.10)
project(Assembler CXX)
set(CMAKE_CXX_STANDARD 17)


add_executable(as main.cc assembler.cc)

set(CMAKE_CXX_FLAGS_DEBUG "-g")

