#include <opencv2/opencv.hpp>
#include "myslam/algorithm.h"
#include "myslam/backend.h"
#include "myslam/config.h"
#include "myslam/feature.h"
#include "myslam/frontend.h"
#include "myslam/g2o_types.h"
#include "myslam/map.h"
#include "myslam/viewer.h"

namespace myslam {

Frontend::Frontend() {
    gftt_ = cv::GFTTDetector::create(Config::Get<int>("num_features"), 0.01, 20); // 150
    num_features_init_ = Config::Get<int>("num_features_init"); // 50
    num_features_ = Config::Get<int>("num_features"); // 150
}

bool Frontend::AddFrame(myslam::Frame::Ptr frame) {
    current_frame_ = frame;

    switch (status_) // status_是前端Frontend的类成员，所以这里判断的是整个前端的状态
    {
    case FrontendStatus::INITING:
        StereoInit(); // bool函数,用来做初始化
        break;
    case FrontendStatus::TRACKING_GOOD:
    case FrontendStatus::TRACKING_BAD:
        // 此处的TRACKING_GOOD和TRACKING_BAD并非跟踪失败或者跟踪成功的含义，这里只是跟踪条件好坏的区别
        // 当跟踪的特征点数目充足时就是good，当特征点数目不足时就是bad，但不管条件恶劣与否，并不涉及失败的问题
        // 当特征点少到不行时，就已经不是good和bad的问题，太少的时候就认为跟踪丢了，设置为LOST，这个LOST可以理解为失败。
        // 所以，LOST之后需要reset系统
        Track();
        break;
    case FrontendStatus::LOST:
        Reset();
        break;
    }

    last_frame_ = current_frame_;
    return true;
}

// 根据上面的Addframe函数，应当在后续实现StereoInit，Track和Reset各个函数
bool Frontend::StereoInit()
{
    int num_features_left = DetectFeatures(); // 一个frame同时包含左右目的图像，这一步是在提取左目特征
    // 通常在左目当中提取特征时特征点数量是一定能保证的。
    int num_coor_features = FindFeaturesInRight(); // 根据左目特征在右目中找对应，虽然左目提取时特征点数量能够保证，但匹配过程无法确保能够在右目图像中为所有的左目特征找到对应，所以这一步最后找到的对应特征数目不一定满足初始化条件。
    if (num_coor_features < num_features_init_) { // 对应数目不足，无法初始化
        return false;
    }

    bool build_map_success = BuildInitMap(); // 初始化成功，则开始建立初始的地图
    if (build_map_success) { // 初始地图建立成功
        status_ = FrontendStatus::TRACKING_GOOD; // 前端状态可有INITING转变为TRACKING_GOOD
        if (viewer_) { // 可视化是否开启
            viewer_->AddCurrentFrame(current_frame_); // 将当前帧加入可视化器
            viewer_->UpdateMap(); // 更新可视化内容以显示当前帧

            // 为什么Track()函数里面只有AddCurrentFrame()
        }
        return true;
    }
    // 如果初始化地图没有建立成功，则会跳过上述if段落，则还是没能完成初始化，返回false
    return false;
}


bool Frontend::Track() {
    if (last_frame_) {
        current_frame_->SetPose(relative_motion_ * last_frame_->Pose());
    }

    int num_track_last = TrackLastFrame();
    tracking_inliers_ = EstimateCurrentPose();

    if (tracking_inliers_ > num_features_tracking_) {
        // tracking good
        status_ = FrontendStatus::TRACKING_GOOD;
    } else if (tracking_inliers_ > num_features_tracking_bad_) {
        // tracking bad
        status_ = FrontendStatus::TRACKING_BAD;
    } else {
        // lost
        status_ = FrontendStatus::LOST;
    }

    InsertKeyframe();
    relative_motion_ = current_frame_->Pose() * last_frame_->Pose().inverse();

    if (viewer_) {
        viewer_->AddCurrentFrame(current_frame_);
    }
    return true;
}

bool Frontend::InsertKeyframe() {
    if (tracking_inliers_ >= num_features_needed_for_keyframe_) {
        // still have enough features, don't insert keyframe
        return false;
    }
    // current frame is a new keyframe
    current_frame_->SetKeyFrame();
    map_->InsertKeyFrame(current_frame_);

    LOG(INFO) << "Set frame " << current_frame_->id_ << " as keyframe " << current_frame_->keyframe_id_;

    SetObservationsForKeyFrame();
    DetectFeatures(); // detect new features

    // track in right image
    FindFeaturesInRight();
    // triangulate map points
    TriangulateNewPoints();
    // update backend because we have a new keyframe
    backend_->UpdateMap();

    if (viewer_) viewer_->UpdateMap();

    return true;
}

void Frontend::SetObservationsForKeyFrame() {
    for (auto &feat : current_frame_->features_left_) {
        auto mp = feat->map_point_.lock();
        if (mp) mp->AddObservation(feat);
    }
}


int Frontend::TriangulateNewPoints() {
    std::vector<SE3> poses{camera_left_->pose(), camera_right_->pose()};
    SE3 cu
}

}