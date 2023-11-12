project(imageBasics)

add_executable(imageBasics imageBasics.cpp)

# 链接 OpenCV库
target_link_libraries(imageBasics ${OpenCV_LIBS})

add_executable(undistortImage undistortImage.cpp)

# 链接 OpenCV 库
target_link_libraries(undistortImage ${OpenCV_LIBS})
