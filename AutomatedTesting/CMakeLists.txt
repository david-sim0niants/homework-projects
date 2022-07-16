cmake_minimum_required(VERSION 3.10)
project(AutomatedtTesting CXX)


add_executable(tester automated_testing.cc main.cc)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
