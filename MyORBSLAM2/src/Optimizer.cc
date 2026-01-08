#include "Optimizer.h"

namespace ORB_SLAM2
{
int Optimizer::PoseOptimization(Frame* pFrame)
{
    return -1;
}


void Optimizer::OptimizeEssentialGraph(Map* pMap, KeyFrame* pLoopKF, KeyFrame* pCurKF,
                                       const LoopClosing::KeyFrameAndPose &NonCorrectedSim3,
                                       const LoopClosing::KeyFrameAndPose &CorrecteedSim3,
                                       const std::map<KeyFrame*, std::set<KeyFrame*>> &LoopConnections,
                                       const bool &bFixScale)
{
    return;
}


int Optimizer::OptimizeSim3(KeyFrame* pKF1, KeyFrame* pKF2, std::vector<MapPoint*> &vpMatches1, g2o::Sim3 &g2oS12, const float th2, const bool bFixScale)
{
    return 0;
}


void Optimizer::LocalBundleAdjustment(KeyFrame* pKF, bool *pbStopFlag, Map* pMP)
{
    return;
}

} // namespace ORB_SLAM2
