#pragma once
#ifndef MAPPOINT_H
#define MAPPOINT_H

#include "KeyFrame.h"
#include "Frame.h"
#include "Map.h"
#include "ORBmatcher.h"

namespace ORB_SLAM2
{
class Frame;
class Map;
class KeyFrame;

class MapPoint
{
public:
    MapPoint(const cv::Mat &Pos, KeyFrame* pRefKF, Map* pMap);
    MapPoint(const cv::Mat &Pos, Map* pMap, Frame* pFrame, const int &idxF);

    // mWorldPos：地图点的世界坐标
    void SetWorldPos(const cv::Mat &Pos);   // mWorldPos的set方法,地图点的世界坐标
    cv::Mat GetWorldPos();                  // mWorldPos的get方法

    cv::Mat GetNormal();                // mNormalVector的get方法，获取平均观测方向
    KeyFrame* GetReferenceKeyFrame();   // mpRefKF的get方法

    // mObservations：与关键帧的观测关系
    void AddObservation(KeyFrame* pKF, size_t idx); // 添加当前地图点对某KeyFrame的观测
    void EraseObservation(KeyFrame* pKF);           // 删除当前地图点对某KeyFrame的观测，mObservations.erase(pKF)
    std::map<KeyFrame*, size_t> GetObservations();  // mObservations的get方法
    bool GetIndexInKeyFrame(KeyFrame* pKF);         // 查询当前地图点在某KeyFrame中的索引
    bool IsInKeyFrame(KeyFrame* pKF);               // 查询当前地图点是否在某KeyFrame中

    int Observations(); // nObs的get方法

    void SetBadFlag();
    bool isBad();

    void Replace(MapPoint* pMP);
    MapPoint* GetReplaced();

    void IncreaseVisible(int n = 1);
    void IncreaseFound(int n = 1);
    float GetFoundRatio();
    inline int GetFound() {
        return mnFound;
    }

    void ComputeDistinctiveDescriptors();   // 计算mDescriptor
    cv::Mat GetDescriptor();

    void UpdateNormalAndDepth();            // 更新平均观测距离和方向

    float GetMinDistanceInvariance();   // mfMinDistance的get方法
    float GetMaxDistanceInvariance();   // mfMaxDistance的get方法
    int PredictScale(const float &currentDist, KeyFrame* pKF);  // 根据某地图点到某关键帧的观测深度估计其在该帧金字塔上的层级
    int PredictScale(const float &currentDist, Frame* pF);      // 根据某地图点到某帧的观测深度估计其在该帧金字塔上的层级

public:
    long unsigned int mnId;
    static long unsigned int nNextId;
    long int mnFirstKFid;               // 第一次观测/生成它的关键帧id
    long int mnFirstFrame;              // 创建该地图点的帧ID(因为关键帧也是帧)
    int nObs;                           // 记录当前地图点被多少相机观测到，单目帧每次观测加1，双目帧每次观测加2

    // Variables used by the tracking
    float mTrackProjX;
    float mTrackProjY;
    float mTrackProjXR;
    bool mbTrackInView;
    int mnTrackScaleLevel;
    float mTrackViewCos;
    long unsigned int mnTrackReferenceForFrame;
    long unsigned int mnLastFrameSeen;

    // Variables used by local mapping
    long unsigned int mnBALocalForKF;
    long unsigned int mnFuseCandidateForKF;

    // Variables used by loop closing
    long unsigned int mnLoopPointForKF;
    long unsigned int mnCorrectedByKF;
    long unsigned int mnCorrectedReference;
    cv::Mat mPosGBA;
    long unsigned int mnBAGlobalForKF;

    static std::mutex mGlobalMutex;

protected:
    // Position in absolute coordinates
    cv::Mat mWorldPos;  // 3D点，地图点的世界坐标

    // 与关键帧的观测关系
    // KeyFrames observing the point and associated index in keyframe
    std::map<KeyFrame*, size_t> mObservations;  // 当前地图点在某KeyFrame中的索引，保存当前地图点在关键帧的索引。


    // 观测尺度相关成员变量
    // Mean viewing direction
    cv::Mat mNormalVector;  // 平均观测方向

    // Best descriptor to fast matching
    cv::Mat mDescriptor;    // 当前地图点的特征描述子（所有描述子的中位数），一个地图点在不同关键帧中对应不同的特征点和描述子
    
    // Reference KeyFrame
    KeyFrame* mpRefKF;  // 当前地图点的参考关键帧

    // Tracking counters
    int mnVisible;  // 在帧中的可视次数
    int mnFound;    // 

    // Bad flag (we do not currently erase MapPoint from memory)
    bool mbBad;
    MapPoint* mpReplaced;
    
    // Scale invariance distances
    float mfMinDistance;    // 平均观测距离的下限，地图点匹配在某特征提取器图像金字塔第0层上的某特征点，观测距离值
    float mfMaxDistance;    // 平均观测距离的上限，地图点匹配在某特征提取器图像金字塔第7层上的某特征点，观测距离值

    Map* mpMap;

    std::mutex mMutexPos;       // mWorldPos的临界锁
    std::mutex mMutexFeatures;  //
};

}

#endif