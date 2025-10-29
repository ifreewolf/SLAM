#pragma once
#ifndef MYSLAM_FEATURE_H
#define MYSLAM_FEATURE_H

#include <memory>
#include <opencv2/features2d.hpp>
#include "myslam/common_include.h"

namespace myslam {

// forward declare
struct Frame;
struct MapPoint;

/**
 * 2D 特征点
 * 在三角化之后会被关联到一个地图点
*/
struct Feature
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    typedef std::shared_ptr<Feature> Ptr;

    std::weak_ptr<Frame> frame_;        // 持有该feature的frame
    cv::KeyPoint position_;             // 2D提取位置
    std::weak_ptr<MapPoint> map_point_; // 关联地图点


    bool is_outlier_ = false;       // 是否为异常点
    bool is_on_left_image_ = true;  // 标识是否在左图提取的特征，false为右图

public:
    Feature() {}
    Feature(std::shared_ptr<Frame> frame, const cv::KeyPoint &kp)
        : frame_(frame), position_(kp) {}
};
}

#endif

// 可以通过一个Feature对象访问持有它的Frame及对应的路标，不过，Frame和MapPoint的实际持有权归地图所有，为了避免shared_ptr产生循环引用，这里使用了weak_ptr