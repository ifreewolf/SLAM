cmake_minimum_required( VERSION 2.8 )
project( geometry)

# 添加Eigen文件头
include_directories( "/usr/include/eigen3" )

add_executable(eigenGeometry eigenGeometry.cpp)