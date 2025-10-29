#include "MapDrawer.h"

namespace ORB_SLAM2
{

MapDrawer::MapDrawer(Map* pMap, const std::string &strSettingPath) : mpMap(pMap)
{
    cv::FileStorage fSettings(strSettingPath, cv::FileStorage::READ);

    mKeyFrameSize      = fSettings["Viewer.KeyFrameSize"];
    mKeyFrameLineWidth = fSettings["Viewer.KeyFrameLineWidth"];
    mGraphLineWidth    = fSettings["Viewer.GraphLineWidth"];
    mPointSize         = fSettings["Viewer.PointSize"];
    mCameraSize        = fSettings["Viewer.CameraSize"];
    mCameraLineWidth   = fSettings["Viewer.CameraLineWidth"];
}

void MapDrawer::SetCurrentCameraPose(const cv::Mat &Tcw)
{
    std::unique_lock<std::mutex> lock(mMutexCamera);
    mCameraPose = Tcw.clone();
}

}