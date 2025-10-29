cmake_minimum_required(VERSION 2.8)
project(vo1)

set(CMAKE_BUILD_TYPE "Release")
add_definitions("-DENABLE_SSE")
set(CMAKE_CXX_FLAGS "-std=c++11 -O2 ${SSE_FLAGS} -msse4")
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 17)

set(OpenCV_DIRS /root/fgs/depLib/opencv/include/opencv4)
# set(Eigen_DIRS /root/fgs/depLib/eigen3/share/eigen3/cmake)
find_package(OpenCV REQUIRED)
find_package(Eigen3)
find_package(Ceres REQUIRED)
find_package(Sophus REQUIRED)

set(G2O_INCLUDE_DIRS /root/fgs/depLib/g2o/include)
set(g2o_DIRS /root/fgs/depLib/g2o/lib/cmake/g2o/)
find_package(g2o REQUIRED)
message("fangguanshou g2o_includes: ${G2O_INCLUDE_DIRS}")
message("fangguanshou g2o_libs: ${G2O_LIBRARYS}")
message("fangguanshou g2o_libs: ${g2o_LIBS}")

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${Sophus_INCLUDE_DIRS}
    ${Eigen3_INCLUDE_DIRS}
    ${G2O_INCLUDE_DIRS}
)

add_executable(orb_cv orb_cv.cpp)
target_link_libraries(orb_cv ${OpenCV_LIBS})

add_executable(orb_self orb_self.cpp)
target_link_libraries(orb_self ${OpenCV_LIBS})

add_executable( pose_estimation_2d2d pose_estimation_2d2d.cpp)
target_link_libraries(pose_estimation_2d2d
    ${G2O_LIBS}
    ${OpenCV_LIBS})

add_executable(triangulation triangulation.cpp)
target_link_libraries(triangulation ${OpenCV_LIBS})

add_executable(pose_estimation_3d2d pose_estimation_3d2d.cpp)
target_link_libraries(pose_estimation_3d2d
    /root/fgs/depLib/g2o/lib/libg2o_core.so
    /root/fgs/depLib/g2o/lib/libg2o_types_slam3d.so
    /root/fgs/depLib/g2o/lib/libg2o_solver_csparse.so
    /root/fgs/depLib/g2o/lib/libg2o_stuff.so
    /root/fgs/depLib/g2o/lib/libg2o_csparse_extension.so
    ${OpenCV_LIBS})
