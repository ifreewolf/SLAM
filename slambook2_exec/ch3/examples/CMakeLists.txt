cmake_minimum_required(VERSION 3.5)
project(examples)

include_directories("/usr/include/eigen3")
add_executable(coordinateTransform coordinateTransform.cpp)