#pragma once
#ifndef LOOPCLOSING_H
#define LOOPCLOSING_H

#include "common.h"


#include "KeyFrame.h"
#include "LocalMapping.h"
#include "Map.h"
#include "ORBVocabulary.h"
#include "Tracking.h"

#include "KeyFrameDatabase.h"

#include "3rdparty/g2o/g2o/types/types_seven_dof_expmap.h"
#include "Sim3Solver.h"

namespace ORB_SLAM2
{

class KeyFrame;
class LocalMapping;
class Map;
class KeyFrameDatabase;
class Tracking;

class LoopClosing
{
public:
    typedef std::pair<std::set<KeyFrame*>, int> ConsistentGroup;
    typedef std::map<KeyFrame*, g2o::Sim3, std::less<KeyFrame*>, Eigen::aligned_allocator<std::pair<const KeyFrame*, g2o::Sim3>>> KeyFrameAndPose;

public:
    LoopClosing(Map* pMap, KeyFrameDatabase* pDB, ORBVocabulary* pVoc, const bool bFixScale);

    void SetTracker(Tracking* pTracker);

    void SetLocalMapper(LocalMapping* pLocalMapper);

    // Main function
    void Run();

    void InsertKeyFrame(KeyFrame *pKF);

    void RequestReset();

    // This function will run in a separate thread
    void RunGlobalBundleAdjustment(unsigned long nLoopKF);

    bool isRunningGBA() {
        std::unique_lock<std::mutex> lock(mMutexGBA);
        return mbRunningGBA;
    }

    bool isFinishedGBA() {
        std::unique_lock<std::mutex> lock(mMutexGBA);
        return mbFinishedGBA;
    }

    void RequestFinish();

    bool isFinished();

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

protected:

    bool CheckNewKeyFrames();

    bool DetectLoop();

    bool ComputeSim3();

    void CorrectLoop();

    void ResetIfRequested();

    bool CheckFinish();

    void SetFinish();

    void SearchAndFuse(const KeyFrameAndPose &CorrectedPosesMap);

    Map* mpMap;

    LocalMapping* mpLocalMapper;
    Tracking*     mpTracker;

    KeyFrameDatabase* mpKeyFrameDB;
    ORBVocabulary* mpORBVocabulary;

    bool mbResetRequested;  // 请求重置标志位
    std::mutex mMutexReset; // mbResetRequested 临界锁

    bool mbFinishRequested;
    bool mbFinished;
    std::mutex mMutexFinish;

    std::list<KeyFrame*> mlpLoopKeyFrameQueue;  // 关键帧队列，进行闭环检测的关键帧队列

    std::mutex mMutexLoopQueue;

    float mnCovisibilityConsistencyTh;

    // Loop detector variables
    KeyFrame* mpCurrentKF;  // 闭环检测，当前帧
    KeyFrame* mpMatchedKF;  // 闭环检测，匹配上的关键帧
    std::vector<ConsistentGroup> mvConsistentGroups;        // 
    std::vector<KeyFrame*> mvpEnoughConsistentCandidates;   // 
    std::vector<KeyFrame*> mvpCurrentConnectedKFs;          // 
    std::vector<MapPoint*> mvpCurrentMatchedPoints;         // 
    std::vector<MapPoint*> mvpLoopMapPoints;                // 
    cv::Mat mScw;
    g2o::Sim3 mg2oScw;

    long unsigned int mLastLoopKFid;    // 最后进行关键帧BA的id

    // Variables related to Global Bundle Adjustment
    bool mbRunningGBA;          // 是否正在进行BA的标识
    bool mbFinishedGBA;         // GBA是否完成的标识
    bool mbStopGBA;             // GBA是否停止的标识
    std::mutex mMutexGBA;       // 上述三个变量的临界锁
    std::thread* mpThreadGBA;   // GBA线程

    // Fix scale in the stereo/RGBD case
    bool mbFixScale;

    bool mnFullBAIdx;
};

}

#endif