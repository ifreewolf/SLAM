#pragma once
#ifndef SYSTEM_H
#define SYSTEM_H

#include "common.h"
#include "ORBVocabulary.h"
#include "KeyFrameDatabase.h"
#include "Map.h"
#include "FrameDrawer.h"
#include "MapDrawer.h"
#include "Tracking.h"
#include "LocalMapping.h"
#include "LoopClosing.h"
#include "Viewer.h"

namespace ORB_SLAM2{

class Viewer;
class Map;
class FrameDrawer;
class Tracking;
class LocalMapping;
class LoopClosing;

class System
{
public:
    // Input sensor
    enum eSensor{
        MONOCULAR=0,
        STEREO=1,
        RGBD=2
    };

public:
    System(const std::string &strVocFile, const std::string &strSettingsFile, const eSensor sensor, const bool bUseViewer = true);

    // Process the given rgbd frame. Depthmap must be registered to the RGB frame.
    // Input image: RGB (CV_8UC3) or grayscale (CV_8U). RGB is converted to grayscale.
    // Returns the camera pose (empty if tracking fails).
    cv::Mat TrackRGBD(const cv::Mat &im, const cv::Mat &depthmap, const double &timestamp);

    // Reset the system (clear map)
    void Reset();

    // All threads will be requested to finish.
    // It waits until all threads have finished.
    // This function must be called before saving the trajectory.
    void Shutdown();

    // Save camera trajectory in the TUM RGB-D dataset format.
    // Only for stereo and RGB-D，this method does not work for monocular.
    // Call first Shutdown()
    // See format details at: http://vision.in.tum.de/data/datasets/rgbd-dataset
    void SaveTrajectoryTUM(const std::string &filename);

    // Save keyframe poses in the TUM RGB-D dataset format.
    // This method works for all sensor input.
    // Call first Shutdown()
    // See format details at: http://vision.in.tum.de/data/datasets/rgbd-dataset
    void SaveKeyFrameTrajectoryTUM(const std::string &filename);

    // Save camera trajectory in the KITTI dataset format.
    // Only for stereo and RGBD. This method does not work for monocular.
    // See format details at: http://www.cvlibs.net/datasets/kitti/eval_odometry.php
    void SaveTrajectoryKITTI(const std::string &filename);

private:
    // Input sensor
    eSensor mSensor; // 传感器类型，可选值 MONOCULAR / STERED / RGBD

    // ORB Vocabulary used for place recognition and feature matching.
    ORBVocabulary* mpVocabulary; // ORB字典，保存ORB描述子聚类结果

    // KeyFrame database for place recognition (relocalization and loop detection).
    KeyFrameDatabase* mpKeyFrameDatabase; // 关键帧数据库，保存ORB描述子聚类结果

    // Map structure that stores the pointers to all KeyFrames and MapPoints.
    Map* mpMap; // 地图

    // Tracker.
    Tracking* mpTracker;    // 追踪器

    // Local Mapper. It manages the local map and performs local bundle adjustment.
    LocalMapping* mpLocalMapper;    // 局部建图器

    // Loop Closer. It searches loops with every new keyframe. If there is a loop it performs
    // a pose graph optimization and full bundle adjustment (in a new thread) afterwards.
    LoopClosing* mpLoopCloser;      // 回环检测器

    // The viewer draws the map and the current camera pose. It users Pangolin
    Viewer* mpViewer;               //  查看器


    FrameDrawer* mpFrameDrawer;     // 帧绘制器
    MapDrawer*   mpMapDrawer;       // 地图绘制器

    // System threads: Local Mapping, Loop Closing, Viewer
    std::thread* mptLocalMapping;   // 局部建图线程
    std::thread* mptLoopClosing;    // 回环检测线程
    std::thread* mptViewer;         // 查看器线程

    // Change mode flags
    std::mutex mMutexMode;
    bool       mbActivateLocalizationMode; // 是否激活仅定位模式模式，如果为true，会关闭LocalMapping线程
    bool       mbDeactivateLocalizationMode; // 是否关闭仅定位模式，如果为true，LocalMapping线程会被释放，关键帧从局部地图中删除

    // Reset flag
    std::mutex mMutexReset; // 线程重置独占锁
    bool       mbReset;     // 线程重置标志位

    // Tracking state
    std::mutex                mMutexState;          // 跟踪状态锁
    int                       mTrackingState;       // 跟踪状态
    std::vector<MapPoint*>    mTrackedMapPoints;    // 跟踪到的地图点
    std::vector<cv::KeyPoint> mTrackedKeyPointsUn;  // 跟踪到的关键点
};

}

#endif