#pragma once

#ifndef MYSLAM_FRAME_H
#define MYSLAM_FRAME_H

#include "myslam/camera.h"
#include "myslam/common_include.h"

namespace myslam {

// forward declare 前向声明
struct MapPoint;
struct Feature;

/**
 * 帧
 * 每一帧分配独立id，关键帧分配关键帧ID
*/
struct Frame {
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW; // 用于在Eigen C++库中启用对齐内存分配。
        typedef std::shared_ptr<Frame> Ptr;

        unsigned long id_ = 0;          // id of this frame
        unsigned long keyframe_id_ = 0; // id of key frame
        bool is_keyframe_ = false;      // 是否为关键帧
        double time_stamp_;             // 时间戳，暂不使用
        SE3 pose_;                      // Tcw形式pose
        std::mutex pose_mutex_;         // Pose数据锁，位姿这个数据会被前后端同时设置或访问，所以需要给函数访问添加锁
        cv::Mat left_img_, right_img_;  // stereo images 

        // 左图提取的特征，与之对应的右图特征，如果右图特征没有，则置为nullptr
        // extracted features in left image
        std::vector<std::shared_ptr<Feature>> features_left_;
        // corresponding features in right image, set to nullptr if no corresponding
        std::vector<std::shared_ptr<Feature>> features_right_;

    public: // data members
        Frame() {} // 无参构造
        Frame(long id, double time_stamp, const SE3 &pose, const Mat &left, const Mat &right); // 有参构造

        // set and get pose, thread safe
        SE3 Pose() {
            std::unique_lock<std::mutex> lck(pose_mutex_);
            return pose_;
        }

        void SetPose(const SE3 &pose) {
            std::unique_lock<std::mutex> lck(pose_mutex_);
            pose_ = pose;
        }

        // 设置关键帧并分配关键帧id
        void SetKeyFrame();

        // 工厂模式，分配id
        static std::shared_ptr<Frame> CreateFrame(); // 用于构建Frame，外部构建Frame的接口
};

}

#endif