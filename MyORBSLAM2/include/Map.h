#pragma once
#ifndef MAP_H
#define MAP_H

#include "common.h"
#include "MapPoint.h"
#include "KeyFrame.h"

namespace ORB_SLAM2
{

class MapPoint;

class Map
{
public:
    Map();

    void clear();

    std::vector<KeyFrame*> mvpKeyFrameOrigins;  // 什么作用？

protected:
    std::set<MapPoint*>    mspMapPoints;            // 地图中地图点集合，set
    std::set<KeyFrame*>    mspKeyFrames;            // 地图中关键帧集合
    std::vector<MapPoint*> mvpReferenceMapPoints;   // 参考地图点集合

    long unsigned int mnMaxKFid;

    // Index related to a big change in the map (loop closure, global BA) // loop closure 闭环，回环
    int mnBigChangeIdx;
};


}

#endif