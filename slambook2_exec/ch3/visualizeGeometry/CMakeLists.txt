cmake_minimum_required(VERSION 3.5)
project(visualizeGeometry)

set(CMAKE_CXX_FLAGS "-std=c++11")

include_directories("/user/include/eigen3")

find_package(Pangolin REQUIRED)
include_directories(${Pangolin_INCLUDE_DIRS})

add_executable(visualizeGeometry visualizeGeometry.cpp)
target_link_libraries(visualizeGeometry ${Pangolin_LIBRARIES})