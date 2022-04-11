cmake_minimum_required(VERSION 3.10)
project(VirtualMachine CXX)


add_executable(vm main.cc virtual_machine.cc)

set(CMAKE_CXX_FLAGS_DEBUG "-g")

