#pragma once
#ifndef MYSLAM_COMMON_INCLUDE_H
#define MYSLAM_COMMON_INCLUDE_H

// std
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <typeinfo>
#include <unordered_map>
#include <vector>

// define the commonly included file to avoid a long include list
#include <Eigen/Core>
#include <Eigen/Geometry>

// typedefs for eigen
// double matricies
typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> MaxXX;
typedef Eigen::Matrix<double, 10, 10> Mat1010;


// for Sophus
#include <sophus/se3.hpp>
#include <sophus/so3.hpp>

typedef Sophus::SE3d SE3;
typedef Sophus::SO3d SO3;

// for cv
#include <opencv2/core/core.hpp>

using cv::Mat;

// glog
#include <glog/logging.h>



#endif