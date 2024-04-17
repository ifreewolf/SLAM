cmake_minimum_required(VERSION 3.5)
project(chapter3)

set(CMAKE_CXX_FLAGS "-std=c++11")

add_subdirectory(useEigen)
add_subdirectory(useGeometry)
add_subdirectory(examples)