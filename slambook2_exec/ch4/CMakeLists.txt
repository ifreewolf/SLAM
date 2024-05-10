cmake_minimum_required(VERSION 3.5)
project(useSophus)

# 为使用 sophus，需要使用find_package命令找到它
set(fmt_DIR /homerep/fgs/Workstation/SLAM/slambook2_exec/3rdparty/fmt/lib/cmake/fmt)
find_package(fmt REQUIRED)
set(Sophus /homerep/fgs/Workstation/SLAM/slambook2_exec/3rdparty/Sophus/share/sophus/cmake)
find_package(Sophus REQUIRED)

# Eigen
include_directories("/usr/include/eigen3")
add_executable(useSophus useSophus.cpp)
target_link_libraries(useSophus Sophus::Sophus)

add_subdirectory(example)