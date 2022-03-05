cmake_minimum_required(VERSION 3.10)
project(HugeInteger CXX)

add_library(huge_integer huge_integer.cpp)

add_executable(huge_int_test main.cpp)
target_link_libraries(huge_int_test huge_integer)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
