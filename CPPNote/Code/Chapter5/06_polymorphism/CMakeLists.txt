cmake_minimum_required(VERSION 3.5)
project(polymorphism)

add_executable(test1 main.cpp)
add_executable(virtualFunc virtualFunc.cpp)
add_executable(virtualDctor virtualDctor.cpp)
add_executable(pureVirtualFunc pureVirtualFunc.cpp)