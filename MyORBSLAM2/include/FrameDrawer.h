#pragma once
#ifndef FRAMEDRAWER_H
#define FRAMEDRAWER_H

#include "common.h"
#include "Map.h"

namespace ORB_SLAM2
{

class FrameDrawer
{
public:
    FrameDrawer(Map* pMap);

protected:
    cv::Mat mIm;
    int     mState;
    Map*    mpMap;

};

}

#endif