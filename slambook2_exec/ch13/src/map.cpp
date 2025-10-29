#include "myslam/map.h"
#include "myslam/feature.h"

namespace myslam {

void Map::InsertKeyFrame(Frame::Ptr frame) {
    current_frame_ = frame;
    if (keyframes_.find(frame->keyframe_id_) == keyframes_.end()) {
        keyframes_.insert(make_pair(frame->keyframe_id_, frame));
        active_keyframes_.insert(make_pair(frame->keyframe_id_, frame));
    } else {
        keyframes_[frame->keyframe_id_] = frame;
        active_keyframes_[frame->keyframe_id_] = frame;
    }

    if (active_keyframes_.size() > num_active_keyframes_) {
        RemoveOldKeyframe();
    }
}


void Map::InsertMapPoint(MapPoint::Ptr map_point) {
    if (landmarks_.find(map_point->id_) == landmarks_.end()) {
        landmarks_.insert(make_pair(map_point->id_, map_point));
        active_landmarks_.insert(make_pair(map_point->id_, map_point));
    } else {
        landmarks_[map_point->id_] = map_point;
        active_landmarks_[map_point->id_] = map_point;
    }
}

/*
删除关键帧Frame，以及关键帧对应的特征点Feature，每个Feature会对应到一个地图点MapPoint
*/
void Map::RemoveOldKeyframe() {
    if (current_frame_ == nullptr) return;
    // 寻找与当前帧最近与最远的两个关键帧
    double max_dis = 0, min_dis = 9999;
    double max_kf_id = 0, min_kf_id = 0;
    auto Twc = current_frame_->Pose().inverse();
    for (auto& kf : active_keyframes_) {
        if (kf.second == current_frame_) continue;
        auto dis = (kf.second->Pose() * Twc).log().norm(); // 这是书上P89页，绝对轨迹误差，ATE = sqrt(norm(log(T^-1T)))
        if (dis > max_dis) {
            max_dis = dis;
            max_kf_id = kf.first;
        }
        if (dis < min_dis) {
            min_dis = dis;
            min_kf_id = kf.first;
        }
    }

    const double min_dis_th = 0.2; // 最近阈值，小于这个阈值才优先删除
    Frame::Ptr frame_to_remove = nullptr;
    if (min_dis < min_dis_th) {
        // 如果存在很近的帧，优先删掉最近的
        frame_to_remove = keyframes_.at(min_kf_id); // unordered_map<int, Frame::Ptr>的at()方法和操作符[]一样
    } else {
        // 删掉最远的
        frame_to_remove = keyframes_.at(max_kf_id);
    }

    LOG(INFO) << "remove keyframe " << frame_to_remove->keyframe_id_;
    // remove keyframe and landmark obervation
    active_keyframes_.erase(frame_to_remove->keyframe_id_);
    for (auto feat : frame_to_remove->features_left_) { // 左图特征
        auto mp = feat->map_point_.lock(); // 特征对应的地图点
        if (mp) {
            mp->RemoveObservation(feat); // 地图点里存放的是特征，可能是多个帧的特征。指定feat删除，意味着删除对应帧的特征
        }
    }
    for (auto feat : frame_to_remove->features_right_) { // 右图特征
        if (feat == nullptr) continue;
        auto mp = feat->map_point_.lock(); // 特征对应的地图点
        if (mp) {
            mp->RemoveObservation(feat);
        }
    }

    CleanMap();

}


/**
 * 目的：
*/
void Map::CleanMap() {
    int cnt_landmark_removed = 0;
    for (auto iter = active_landmarks_.begin(); iter != active_landmarks_.end();) {
        if (iter->second->observed_times_ == 0) { // 第一个观察到对应这个地图点的特征，不删除
            iter = active_landmarks_.erase(iter);
            cnt_landmark_removed++;
        } else {
            ++iter;
        }
    }
    LOG(INFO) << "Removed " << cnt_landmark_removed << " active landmarks";
}
}