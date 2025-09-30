#pragma once
#ifndef VIEWER_H
#define VIEWER_H

#include "common.h"
#include "System.h"
#include "FrameDrawer.h"
#include "Tracking.h"
#include "MapDrawer.h"

namespace ORB_SLAM2
{

class Tracking;
class FrameDrawer;
class MapDrawer;
class System;

class Viewer
{
public:
    Viewer(System* pSystem, FrameDrawer* pFrameDrawer, MapDrawer* pMapDrawer, Tracking* pTracking, const std::string& strSettingPath);

    // Main thread function. Draw points, keyframes, the current camera
    void Run();

    void RequestStop();

    bool isStopped();

    void Release();

private:
    // 1/fps in ms
    double mT;
    float  mImageWidth, mImageHeight;
    float  mViewpointX, mViewpointY, mViewpointZ, mViewpointF;

    bool mbFinished;
    bool mbStopped;
    bool mbStopRequested;

    std::mutex mMutexStop;  // 修改 mbStopped、mbStopRequested 的临界锁
};

}

#endif