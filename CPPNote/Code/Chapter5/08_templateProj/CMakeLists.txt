cmake_minimum_required(VERSION 3.5)
project(templateProj)

set(CMAKE_CXX_FLAGS "-std=c++17")

include_directories(
    ./
)

add_executable(${PROJECT_NAME} main.cpp person.cpp)