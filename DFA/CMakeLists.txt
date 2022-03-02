cmake_minimum_required(VERSION 3.10)
project(DFA CXX)


add_executable(dfa dfa.cpp main.cpp)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
