#include "FrameDrawer.h"

namespace ORB_SLAM2
{

FrameDrawer::FrameDrawer(Map* pMap) : mpMap(pMap)
{
    // mState = Tracking::SYSTEM_NOT_READY;
    mIm = cv::Mat(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
}

}