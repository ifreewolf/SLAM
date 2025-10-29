#pragma once
#ifndef FRAMEDRAWER_H
#define FRAMEDRAWER_H

#include "common.h"
#include "Tracking.h"
#include "Map.h"

namespace ORB_SLAM2
{

class Tracking;

class FrameDrawer
{
public:
    FrameDrawer(Map* pMap);

    // Update info from the last processed frame.
    void Update(Tracking *pTracker);

    // Draw last processed frame.
    cv::Mat DrawFrame();

protected:
    void DrawTextInfo(cv::Mat &im, int nState, cv::Mat &imText);

    // Info of the frame to be drawn
    cv::Mat mIm;
    int N;
    std::vector<cv::KeyPoint> mvCurrentKeys;
    std::vector<bool> mvbMap;
    std::vector<bool> mvbVO;
    bool mbOnlyTracking;
    int mnTracked;
    int mnTrackedVO;
    std::vector<cv::KeyPoint> mvIniKeys;
    std::vector<int> mvIniMatches;
    int     mState;

    Map*    mpMap;

    std::mutex mMutex;

};

}   // namespace ORB_SLAM

#endif