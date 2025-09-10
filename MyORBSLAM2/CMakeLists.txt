cmake_minimum_required(VERSION 3.5)
project(ORB_SLAM2)

set(CMAKE_BUILD_TYPE Debug)

# 全局定义_DEBUG宏
add_definitions(-D_DEBUG)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
ENDIF()

MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3 -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -march=native")

# Check C++11 or C++0x support

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)


find_package(OpenCV REQUIRED) # QUIET安静，尝试寻找OpenCV，找不到也不显示错误信息
if (NOT OpenCV_FOUND)
    find_package(OpenCV 2.4.3 QUIET)
    if (NOT OpenCV_FOUND)
        message(FATAL_ERROR "OpenCV > 2.4.3 not found.")
    endif()
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)
    
add_library(${PROJECT_NAME} SHARED
    src/System.cc
)
target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
)
    
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

add_executable(Test 
    main.cpp
)

add_executable(rgbd_tum
    Examples/RGBD/rgbd_tum.cc
)
target_link_libraries(rgbd_tum
    ${PROJECT_NAME}
)
