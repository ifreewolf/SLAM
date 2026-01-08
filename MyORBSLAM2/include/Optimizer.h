#pragma once
#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "common.h"
#include "Map.h"
#include "MapPoint.h"
#include "KeyFrame.h"
#include "Frame.h"
#include "3rdparty/g2o/g2o/types/types_seven_dof_expmap.h"

#include "LoopClosing.h"

namespace ORB_SLAM2
{
class Map;
class KeyFrame;
class LoopClosing;

class Optimizer
{
public:
    typedef std::map<KeyFrame*, g2o::Sim3, std::less<KeyFrame*>, Eigen::aligned_allocator<std::pair<const KeyFrame*, g2o::Sim3>>> KeyFrameAndPose;
public:
    void static LocalBundleAdjustment(KeyFrame* pKF, bool *pbStopFlag, Map* pMP);
    
    int static PoseOptimization(Frame* pFrame);

    // If bFixScale is true, optimize SE3 (stereo, rgbd), 7DoF otherwise (mono)
    void static OptimizeEssentialGraph(Map* pMap, KeyFrame* pLoopKF, KeyFrame* pCurKF,
                                       const KeyFrameAndPose &NonCorrectedSim3,
                                       const KeyFrameAndPose &CorrecteedSim3,
                                       const std::map<KeyFrame*, std::set<KeyFrame*>> &LoopConnections,
                                       const bool &bFixScale);

    // If bFixScale is true, optimize SE3 (stereo,rgbd), Sim3 otherwise (mono)
    static int OptimizeSim3(KeyFrame* pKF1, KeyFrame* pKF2, std::vector<MapPoint*> &vpMatches1, g2o::Sim3 &g2oS12, const float th2, const bool bFixScale);
};
}

#endif
