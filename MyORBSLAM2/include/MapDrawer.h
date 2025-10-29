#pragma once
#ifndef MAPDRAWER_H
#define MAPDRAWER_H

#include "common.h"
#include "Map.h"

namespace ORB_SLAM2
{

class MapDrawer
{
public:
    MapDrawer(Map* pMap, const std::string &strSettingPath);

    Map* mpMap;

    void SetCurrentCameraPose(const cv::Mat &Tcw);

private:
    float mKeyFrameSize;
    float mKeyFrameLineWidth;
    float mGraphLineWidth;
    float mPointSize;
    float mCameraSize;
    float mCameraLineWidth;

    cv::Mat mCameraPose;

    std::mutex mMutexCamera;
};

}

#endif