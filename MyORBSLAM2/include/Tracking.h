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
#include "ORBmatcher.h"
#include "Optimizer.h"
#include "PnPsolver.h"
#include "Optimizer.h"

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

    // Initialization Variables (Monocular)
    std::vector<int> mvIniLastMatches;
    std::vector<int> mvIniMatches;  // 单目初始化中参考帧与当前帧的匹配关系
    Frame mInitialFrame;            // 单目初始化参考帧（实际上就是前一帧）

    // True if local mapping is deactivate and we are performing only localizaiton
    bool mbOnlyTracking;

    // Lists used to recover the full camera trajectory at the end of the execution
    std::list<cv::Mat>   mlRelativeFramePoses;  // 存储的是相对位姿，在执行完程序之后，用于恢复相机的轨迹
    std::list<KeyFrame*> mlpReferences;         // 参考关键帧列表
    std::list<double>    mlFrameTimes;          // 帧时间列表
    std::list<bool>      mlbLost;               // 帧是否丢失的列表

protected:

    // Main tracking function. It is independent of the input sensor.
    void Track();

    // Map initialization for stereo and RGBD
    void StereoInitialization();

    // Map initialization for monocular
    void MonocularInitialization();     // 单目相机初始化
    void CreateInitialMapMonocular();   // 单目初始化成功后建立初始局部地图

    void CheckReplacedInLastFrame();
    bool TrackReferenceKeyFrame();
    void UpdateLastFrame();
    bool TrackWithMotionModel();

    bool Relocalization();

    void UpdateLocalMap();
    void UpdateLocalPoints();
    void UpdateLocalKeyFrames();

    bool TrackLocalMap();
    void SearchLocalPoints();

    bool NeedNewKeyFrame();
    void CreateNewKeyFrame();

    // In case of performing only localization, this flag is true when there are no matches to points in the map.   在执行仅定位操作时，若当前帧与地图特征点无匹配，则该标志为真。
    // Still tracking continue if there are enough matches with temporal points.    此时系统会继续追踪（若存在足够多的临时特征点匹配）
    // In that case we are doing visual odometry. The System wil try to do relocalization to recover "zero-drift" localization to the map.  此时系统将执行视觉里程计功能,系统会尝试通过重定位操作恢复"零漂移"的定位状态（即重新与地图对齐）。
    bool mbVO;

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

    // Local Map
    KeyFrame* mpReferenceKF;                    // 参考关键帧，初始化成功的帧会被设为参考关键帧
    std::vector<KeyFrame*> mvpLocalKeyFrames;   // 局部关键帧列表，初始化成功后向其中添加局部关键帧
    std::vector<MapPoint*> mvpLocalMapPoints;   // 局部地图点列表，初始化成功后向其中添加局部地图点

    // System
    System* mpSystem;

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
    FrameDrawer* mpFrameDrawer;
    MapDrawer* mpMapDrawer;
    
    // For RGBD inputs only. For some datasets (e.g. TUM) the depthmap values are scaled. scaled:按比例缩放、归一化
    float mDepthMapFactor; // 深度相机disparity转化为depth时的因子

    // Current matches in frame
    int mnMatchesInliers;

    // Last Frame, KeyFrame and Relocalization Info
    Frame mLastFrame;
    KeyFrame* mpLastKeyFrame;
    unsigned int mnLastKeyFrameId;
    unsigned int mnLastRelocFrameId;

    // Motion Model
    cv::Mat mVelocity;

    // Color order (true RGB, false BGR, ignored if grayscale)
    bool mbRGB;

    std::list<MapPoint*> mlpTemporalPoints;
};

}

#endif