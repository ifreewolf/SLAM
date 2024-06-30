cmake_minimum_required(VERSION 3.5)
project(exception1)

set(CMAKE_CXX_FLAGS "-std=c++14")

add_executable(${PROJECT_NAME} main.cpp)
add_executable(throw1 throw1.cpp)
add_executable(unwinding unwinding.cpp)
add_executable(exceptInterface exceptInterface.cpp)
add_executable(exceptAlive exceptAlive.cpp)
add_executable(exceptClass exceptClass.cpp)
add_executable(exceptCutom exceptCutom.cpp)