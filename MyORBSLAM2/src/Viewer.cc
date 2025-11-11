#include "Viewer.h"

namespace ORB_SLAM2
{

Viewer::Viewer(System* pSystem, FrameDrawer* pFrameDrawer, MapDrawer* pMapDrawer, Tracking* pTracking, const std::string& strSettingPath):
    mpSystem(pSystem),
    mpFrameDrawer(pFrameDrawer),
    mpMapDrawer(pMapDrawer),
    mpTracker(pTracking),
    mbFinishRequested(false),
    mbFinished(true),
    mbStopped(true),
    mbStopRequested(false)
{
    cv::FileStorage fSettings(strSettingPath, cv::FileStorage::READ);

    float fps = fSettings["Camera.fps"];
    if (fps < 1) {
        fps = 30;
    }

    mT = 1e3 / fps;

    mImageWidth  = fSettings["Camera.width"];
    mImageHeight = fSettings["Camera.height"];
    if (mImageWidth < 1 || mImageHeight < 1) {
        mImageWidth  = 640;
        mImageHeight = 480;
    }

    mViewpointX = fSettings["Viewer.ViewpointX"];
    mViewpointY = fSettings["Viewer.ViewpointY"];
    mViewpointZ = fSettings["Viewer.ViewpointZ"];
    mViewpointF = fSettings["Viewer.ViewpointF"];
}

void Viewer::Run()
{
    mbFinished = false;
    mbStopped  = false;

    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}


void Viewer::RequestFinish()
{
    std::unique_lock<std::mutex> lock(mMutexFinish);
    mbFinishRequested = true;
}


void Viewer::RequestStop()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    if (!mbStopped) {
        mbStopRequested = true;
    }
}


bool Viewer::isFinished()
{
    std::unique_lock<std::mutex> lock(mMutexFinish);
    if (!mbStopped) {
        mbStopRequested = true;
    }
}


bool Viewer::isStopped()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    return mbStopped;
}

void Viewer::Release()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    mbStopped = false;
}

}