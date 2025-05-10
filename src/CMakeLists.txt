cmake_minimum_required(VERSION 3.5)
project(routing)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(pybind11)
pybind11_add_module(routing binding.cpp functions.cpp)
