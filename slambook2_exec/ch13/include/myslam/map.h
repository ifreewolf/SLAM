#pragma once
#ifndef MAP_H
#define MAP_H

#include "myslam/common_include.h"
#include "myslam/frame.h"
#include "myslam/mappoint.h"

/**
 * 
 * 地图以散列形式记录了所有的关键帧和对应的路标点，同时维护一个被激活的关键帧和地图点。这里激活的概念即我们所谓的窗口，它会随着时间往前推动。
 * 
 * 后端将从地图中取出激活的关键帧、路标点进行优化，忽略其余的部分，达到控制优化规模的效果。
 * 
 * 激活的策略：去除最旧的关键帧而保持时间上最新的若干个关键帧，在这里只保留最新的7个关键帧。
 * 
 * 通常是两帧图像(左图和右图)作为一个整体，每帧图像对应多个特征(Feature)，一个特征对应一个地图点(MapPoint)，在地图点这个类中包含了多个特征，这些特征都是对应这个地图点的。
 * Map类管理多组帧(一组为两帧，左图和右图)和多个特征。这些特征来自于不同的图像帧。
 * 
 * 1. 删除不活跃的关键帧
 *      a. 根据绝对轨迹误差公式计算所有帧位姿与当前帧的距离，优先删除最近的，其次删除最远的。
 *      b. 分别访问帧对应的左右图像的特征，每一帧有多个特征，同时每个特征都会对应上一个地图点。根据当前特征调用地图点MapPoint的清理方法来清理地图点
 *      c. 遍历特征对应的地图点，然后找到对应的特征进行删除，同时特征指向的地图点指针也将被释放。
*/

namespace myslam {
/**
 * @brief 地图
 * 和地图的交互：前端调用InsertKeyframe和InsertMapPoint插入新帧和地图点，后端维护地图的结构，判定outlier/剔除等等
*/
class Map {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    typedef std::shared_ptr<Map> Ptr;
    typedef std::unordered_map<unsigned long, MapPoint::Ptr> LandmarksType;
    typedef std::unordered_map<unsigned long, Frame::Ptr> KeyframesType;

    Map() {}

    // 增加一个关键帧
    void InsertKeyFrame(Frame::Ptr frame);
    // 增加一个地图点
    void InsertMapPoint(MapPoint::Ptr map_point);

    // 获取所有地图点
    LandmarksType GetAllMapPoints() {
        std::unique_lock<std::mutex> lck(data_mutex_);
        return landmarks_;
    }

    // 获取所有关键帧
    KeyframesType GetAllKeyFrames() {
        std::unique_lock<std::mutex> lck(data_mutex_);
        return keyframes_;
    }

    // 获取激活地图点
    LandmarksType GetActiveMapPoints() {
        std::unique_lock<std::mutex> lck(data_mutex_);
        return active_landmarks_;
    }

    // 获取激活关键帧
    KeyframesType GetActiveKeyFrames() {
        std::unique_lock<std::mutex> lck(data_mutex_);
        return active_keyframes_;
    }

    // 清理map中观测数量为零的点
    void CleanMap();

private:
    // 将旧的关键帧置为不活跃状态
    void RemoveOldKeyframe();

    std::mutex data_mutex_;
    LandmarksType landmarks_;           // all landmarks
    LandmarksType active_landmarks_;    // active landmarks
    KeyframesType keyframes_;           // all key-frames
    KeyframesType active_keyframes_;    // active key-frames

    Frame::Ptr current_frame_ = nullptr;

    // settings
    int num_active_keyframes_ = 7;  // 激活的关键帧数量
};
}

#endif