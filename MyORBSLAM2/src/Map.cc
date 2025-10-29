#include "Map.h"

namespace ORB_SLAM2
{

Map::Map(): mnMaxKFid(0), mnBigChangeIdx(0)
{
    
}

void Map::AddKeyFrame(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lock(mMutexMap);
    mspKeyFrames.insert(pKF);
    if (pKF->mnId > mnMaxKFid) {
        mnMaxKFid = pKF->mnId;
    }
}


void Map::AddMapPoint(MapPoint* pMP)
{
    std::unique_lock<std::mutex> lock(mMutexMap);
    mspMapPoints.insert(pMP);
}


long unsigned int Map::MapPointsInMap()
{
    std::unique_lock<std::mutex> lock(mMutexMap);
    return mspMapPoints.size();
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


std::vector<MapPoint*> Map::GetAllMapPoints()
{
    std::unique_lock<std::mutex> lock(mMutexMap);
    return std::vector<MapPoint*>(mspMapPoints.begin(), mspMapPoints.end());
}


void Map::SetReferenceMapPoints(const std::vector<MapPoint*> &vpMPs)
{
    std::unique_lock<std::mutex> lock(mMutexMap);
    mvpReferenceMapPoints = vpMPs;
}


void Map::EraseMapPoint(MapPoint* pMP)
{
    std::unique_lock<std::mutex> lock(mMutexMap);
    mspMapPoints.erase(pMP);
}


void Map::EraseKeyFrame(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lock(mMutexMap);
    mspKeyFrames.erase(pKF);
}


long unsigned int Map::KeyFramesInMap()
{
    std::unique_lock<std::mutex> lock(mMutexMap);
    return mspKeyFrames.size();
}
}