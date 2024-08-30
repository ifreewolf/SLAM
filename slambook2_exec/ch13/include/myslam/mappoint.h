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
 * 
 * MapPoint最主要的信息是它的3D位置，即pos_变量，同样需要对它上锁。它的observation_变量记录了自己被哪些Feature观察，因为Feature可能被判断为outlier，所以observation部分发生改动时也需要锁定。
*/
struct MapPoint {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    typedef std::shared_ptr<MapPoint> Ptr;

    unsigned long id_ = 0;      // id
    bool is_outlier_ = false;   // 异常点
    Vec3 pos_ = Vec3::Zero();   // Position in world，世界位姿
    std::mutex data_mutex_;     // 数据读写互斥锁
    int observed_times_ = 0;    // being ovserved by feature matching algo, 有几个特征
    std::list<std::weak_ptr<Feature>> observations_; // 一个地图点，对应的所有图像帧特征

    MapPoint() {}

    MapPoint(long id, Vec3 position);

    Vec3 Pos() {
        std::unique_lock<std::mutex> lck(data_mutex_);
        return pos_;
    }

    void SetPos(const Vec3 &pos) {
        std::unique_lock<std::mutex> lck(data_mutex_);
        pos_ = pos;
    }

    void AddObservation(std::shared_ptr<Feature> feature) {
        std::unique_lock<std::mutex> lck(data_mutex_);
        observations_.push_back(feature);
        observed_times_++;
    }

    void RemoveObservation(std::shared_ptr<Feature> feat);

    std::list<std::weak_ptr<Feature>> GetObs() {
        std::unique_lock<std::mutex> lck(data_mutex_);
        return observations_;
    }

    // factory function
    static MapPoint::Ptr CreateNewMappoint();
};
}

#endif