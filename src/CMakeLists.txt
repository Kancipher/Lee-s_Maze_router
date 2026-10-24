# Compatibility entry point for the historical `cmake -S src` instructions.
cmake_minimum_required(VERSION 3.20)
project(MazeRouterCompatibility LANGUAGES CXX)
add_subdirectory(.. router-build)
