set(fmt_DIR /home/fgs/Workstations/SLAM/slambook2_exec/3rdparty/fmt/include/fmt)
find_package(fmt REQUIRED)
find_package(Sophus REQUIRED)
include_directories(${Sophus_INCLUDE_DIRS})

find_package(Pangolin REQUIRED)

add_executable(joinMap joinMap.cpp)
target_link_libraries(joinMap ${OpenCV_LIBS} ${Pangolin_LIBRARIES})