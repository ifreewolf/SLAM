cmake_minimum_required(VERSION 3.5)
project(template)

set(CMAKE_CXX_FLAGS "-std=c++17")

add_executable(test1 test1.cpp)
add_executable(test2 test2.cpp)
add_executable(templateSort templateSort.cpp)
add_executable(templateTest templateTest.cpp)
add_executable(classTemplate classTemplate.cpp)
add_executable(classTemplateClass classTemplateClass.cpp)