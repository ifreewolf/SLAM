cmake_minimum_required(VERSION 3.5)
project(test)

add_executable(${PROJECT_NAME} main.cpp)
add_executable(diamondInherit diamondInherit.cpp)
add_executable(multiInherit multiInherit.cpp)
add_executable(virtualInherti virtualInherti.cpp)