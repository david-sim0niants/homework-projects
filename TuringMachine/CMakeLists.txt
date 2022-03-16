cmake_minimum_required(VERSION 3.10)
project(TuringMachine CXX)


add_executable(turing_machine turing_machine.cc main.cc)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
