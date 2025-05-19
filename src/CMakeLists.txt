 cmake_minimum_required(VERSION 3.5)
 project(routing)

 set(CMAKE_CXX_STANDARD 17)

 add_subdirectory(pybind11)
 include_directories(${CMAKE_CURRENT_SOURCE_DIR}/pybind11/include)

 find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
 include_directories(${Python3_INCLUDE_DIRS})
 link_directories(${Python3_LIBRARY_DIRS})

# was building two modules in one .pyd – causes duplicate PyInit
#pybind11_add_module(routing binding.cpp functions.cpp)
# build just one module from functions.cpp
pybind11_add_module(routing functions.cpp)
