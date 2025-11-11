#pragma once
#ifndef LOCALMAPPING_H
#define LOCALMAPPING_H

#include "common.h"
#include "Map.h"
#include "LoopClosing.h"
#include "Tracking.h"

namespace ORB_SLAM2
{
class Map;
class LoopClosing;
class Tracking;

class LocalMapping
{
public:
    LocalMapping(Map* pMap, const bool bMonocular);

    void SetLoopCloser(LoopClosing* pLoopCloser);

    void SetTracker(Tracking* pTracker);

    // Main function
    void Run();

    void InsertKeyFrame(KeyFrame* pKF);

    // Thread Synch
    void RequestStop();     // 请求停止
    void RequestReset();    // 请求重置
    bool isStopped();       // 是否停止
    void Release();         // LocalMapping线程释放
    bool stopRequested();
    bool AcceptKeyFrames();
    bool SetNotStop(bool flag);

    void InterruptBA();

    void RequestFinish();
    bool isFinished();

    int KeyframesInQueue() {
        std::unique_lock<std::mutex> lock(mMutexNewKFs);
        return mlNewKeyFrames.size();
    }

protected:



    bool mbMonocular;

    Map* mpMap;

    bool mbFinishRequested; // 查询是否完成的标识
    bool mbFinished;        // LoalMapping线程是否完成的标识

    LoopClosing* mpLoopCloser;
    Tracking*    mpTracker;

    std::mutex mMutexStop;      // 停止LocalMapping线程的独占锁
    std::mutex mMutexNewKFs;    // 新关键帧链表的独占锁
    std::mutex mMutexFinish;    // LocalMaping线程完成独占锁
    std::mutex mMutexReset;     // LocalMapping线程重置独占锁

    std::list<KeyFrame*> mlNewKeyFrames; // 关键帧链表

    bool mbStopped;         // 线程停止标志位
    bool mbStopRequested;   // 线程停止请求标志位
    bool mbAbortBA;         // BA优化停止标志位
    bool mbResetRequested;  // 线程重置请求标志位
    bool mbNotStop;         // 线程不停止标志位

    bool mbAcceptKeyFrames;
    std::mutex mMutexAccept;
};
}

#endif