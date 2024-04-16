cmake_minimum_required(VERSION 3.5)
project(geometry)

include_directories("/usr/include/eigen3")

add_executable(${PROJECT_NAME}
    eigenGeometry.cpp
)