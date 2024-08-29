#pragma once
#ifndef MYSLAM_MAPPOINT_H
#define MYSLAM_MAPPOINT_H

namespace myslam {

// forward declare
struct Frame;
struct Feature;

/**
 * 路标点类
 * 特征点在三角化之后形成路标点
*/
struct MapPoint {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    typedef std::shared_ptr<MapPoint> Ptr;

    unsigned long id_ = 0;      // id
    bool is_outlier_ = false;
    Vec3 pos_ = Vec3::Zero();   // Position in world
    std::mutex data_mutex_;
    int observed_times_ = 0;    // being ovserved by feature matching algo
    std::list<std::weak_ptr<Feature>> observations_;
};
}

#endif