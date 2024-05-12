project(imageBasics)

add_executable(${PROJECT_NAME} imageBasics.cpp)

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

add_executable(undistortImage undistortImage.cpp)
target_link_libraries(undistortImage ${OpenCV_LIBS})