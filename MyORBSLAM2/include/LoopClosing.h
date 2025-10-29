#pragma once
#ifndef LOOPCLOSING_H
#define LOOPCLOSING_H

#include "common.h"
#include "Map.h"
#include "ORBVocabulary.h"
#include "KeyFrameDatabase.h"
#include "LocalMapping.h"
#include "Tracking.h"

namespace ORB_SLAM2
{

class Tracking;
class LocalMapping;
class KeyFrameDatabase;

class LoopClosing
{
public:
    LoopClosing(Map* pMap, KeyFrameDatabase* pDB, ORBVocabulary* pVoc, const bool bFixScale);

    void SetTracker(Tracking* pTracker);

    void SetLocalMapper(LocalMapping* pLocalMapper);

    // Main function
    void Run();

    void RequestReset();

protected:
    Map* mpMap;
    bool mbFinished;

    LocalMapping* mpLocalMapper;
    Tracking*     mpTracker;

    bool mbResetRequested;  // 请求重置标志位
    std::mutex mMutexReset; // mbResetRequested 临界锁
};

}

#endif