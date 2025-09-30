#pragma once
#ifndef TRACKING_H
#define TRACKING_H

#include "common.h"
#include "System.h"
#include "ORBVocabulary.h"
#include "KeyFrameDatabase.h"
#include "MapDrawer.h"
#include "Frame.h"
#include "LoopClosing.h"
#include "LocalMapping.h"
#include "Viewer.h"
#include "Initializer.h"
#include "ORBextractor.h"

namespace ORB_SLAM2
{
class Viewer;
class System;
class FrameDrawer;
class Map;
class LoopClosing;
class LocalMapping;

class Tracking
{
public:
    Tracking(System* pSys, ORBVocabulary* pVoc, FrameDrawer* pFrameDrawer, MapDrawer* pMapDrawer, Map* pMap,
             KeyFrameDatabase* pKFDB, const std::string &strSettingPath, const int sensor);
    
    // Preprocess the input and call Track(). Extract features and performs stereo matching
    cv::Mat GrabImageStereo();
    cv::Mat GrabImageRGBD(const cv::Mat &imRGB, const cv::Mat &imD, const double &timestamp);
    cv::Mat GrabImageMonocular();

    void SetLocalMapper(LocalMapping* pLocalMapper);
    void SetLoopClosing(LoopClosing* pLoopClosing);
    void SetViewer(Viewer* pViewer);

    // Use this function if you have deactivated local mapping and you only want to localize the camera
    void InformOnlyTracking(const bool &flag);

    void Reset();   // 线程重置

public:
    // Tracking status 
    enum eTrackingState{        // 用于表示追踪状态
        SYSTEM_NOT_READY=-1,    // 系统还没有准备好，一般就是在启动后加载配置文件和词典文件时候的状态
        NO_IMAGES_YET=0,         // 还没有接收到输入图像
        NOT_INITIALIZED=1,      // 接收到图像但未初始化成功
        OK=2,                   // 跟踪成功
        LOST=3                  // 追踪失败
    };

    eTrackingState mState;              // 当前帧的跟踪状态
    eTrackingState mLastProcessedState; // 前一帧的跟踪状态

    // Input sensor
    int mSensor;

    // Current Frame
    Frame   mCurrentFrame; // 当前帧
    cv::Mat mImGray;

    // True if local mapping is deactivate and we are performing only localizaiton
    bool mbOnlyTracking;

    // Lists used to recover the full camera trajectory at the end of the execution
    std::list<cv::Mat>   mlRelativeFramePoses;  // 存储的是相对位姿，在执行完程序之后，用于恢复相机的轨迹
    std::list<KeyFrame*> mlpReferences;         // 参考关键帧列表
    std::list<double>    mlFrameTimes;          // 帧时间列表
    std::list<bool>      mlbLost;               // 丢失帧的列表

protected:

    // Main tracking function. It is independent of the input sensor.
    void Track();

    // Other Thread Pointers
    LocalMapping* mpLocalMapper;
    LoopClosing*  mpLoopClosing;

    // ORB
    ORBextractor* mpORBextractorLeft;   // ORB特征提取器，左图
    ORBextractor* mpORBextractorRight;  // ORB特征提取器，右图
    ORBextractor* mpIniORBextractor;    // ORB特征提取器，初始化的时候使用，仅用于单目

    // BoW
    KeyFrameDatabase* mpKeyFrameDB;
    ORBVocabulary*    mpORBVocabulary;

    // Initialization (only for monocular)
    Initializer* mpInitializer; // 单目初始化器

    // Map
    Map* mpMap;

    // Calibration matrix
    cv::Mat mK;         // 内参矩阵
    cv::Mat mDistCoef;  // 畸变矫正系数矩阵，[k1, k2, p1, p2, k3],k3有些相机没有
    float   mbf;        // b是基线，f是焦距，距离z=fb/d, d是视差

    // New KeyFrame rules (according to fps)
    int mnMinFrames;
    int mnMaxFrames;

    // Threshold close/far points 近点和远点的阈值
    // Points seen as close by the stereo/RGBD sensor are considered reliable and inserted from just one frame. 立体视觉/RGBD传感器‌检测到的‌近距离点‌被视为可靠数据，仅需‌单帧匹配‌即可插入
    // Far points requier a match in two keyframes. 而‌远距离点‌则需在‌两帧关键帧‌中匹配确认‌
    float mfThDepth;

    // Drawers
    Viewer* mpViewer;
    
    // For RGBD inputs only. For some datasets (e.g. TUM) the depthmap values are scaled. scaled:按比例缩放、归一化
    float mDepthMapFactor; // 深度相机disparity转化为depth时的因子

    // Color order (true RGB, false BGR, ignored if grayscale)
    bool mbRGB;


};

}

#endif