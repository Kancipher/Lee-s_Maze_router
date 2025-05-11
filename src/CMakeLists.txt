cmake_minimum_required(VERSION 3.5)
project(routing)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(pybind11)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/pybind11/include)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
include_directories(${Python3_INCLUDE_DIRS})
link_directories(${Python3_LIBRARY_DIRS})

pybind11_add_module(routing binding.cpp functions.cpp)