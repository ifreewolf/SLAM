#include "Map.h"

namespace ORB_SLAM2
{

Map::Map(): mnMaxKFid(0), mnBigChangeIdx(0)
{
    
}

void Map::clear()
{
    for (std::set<MapPoint*>::iterator sit = mspMapPoints.begin(), send = mspMapPoints.end(); sit != send; sit++) {
        delete *sit;
    }

    for (std::set<KeyFrame*>::iterator sit = mspKeyFrames.begin(), send = mspKeyFrames.end(); sit != send; sit++) {
        delete *sit;
    }

    mspMapPoints.clear();
    mspKeyFrames.clear();
    mnMaxKFid = 0;
    mvpReferenceMapPoints.clear();
    mvpKeyFrameOrigins.clear();
}

}