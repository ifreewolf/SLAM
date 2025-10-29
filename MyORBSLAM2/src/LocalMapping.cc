#include "LocalMapping.h"

namespace ORB_SLAM2
{
LocalMapping::LocalMapping(Map* pMap, const bool bMonocular):
    mbMonocular(bMonocular), mpMap(pMap), mbAbortBA(false), mbStopRequested(false), mbFinished(true),
    mbResetRequested(false)
{
    /**
     * mbStopRequested  外部线程调用，为true，表示外部线程请求停止LocalMapping线程
     * mbAbortBA        是否终止BA优化流程
     */

}


void LocalMapping::SetLoopCloser(LoopClosing* pLoopCloser)
{
    mpLoopCloser = pLoopCloser;
}


void LocalMapping::SetTracker(Tracking* pTracker)
{
    mpTracker = pTracker;
}


void LocalMapping::Run()
{
    mbFinished = false;
    while (1) {
        static int i = 0;
        // std::cout << "LocalMapping i = " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        i += 1;
    }
}


void LocalMapping::RequestStop()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    mbStopRequested = true;
    std::unique_lock<std::mutex> lock2(mMutexNewKFs);
    mbAbortBA = true; // 放弃Bundle Adjustment
}


bool LocalMapping::isStopped()
{
    std::unique_lock<std::mutex>(mMutexStop);
    return mbStopped;
}

void LocalMapping::Release()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    std::unique_lock<std::mutex> lock2(mMutexFinish);
    if (mbFinished) {   // LocalMapping线程是否已经完成
        return;
    }
    mbStopped = false;
    mbStopRequested = false;
    for (std::list<KeyFrame*>::iterator lit = mlNewKeyFrames.begin(), lend = mlNewKeyFrames.end(); lit != lend; lit++) {
        delete *lit;
    }
    mlNewKeyFrames.clear();

    std::cout << "Local Mapping RELEASE" << std::endl;
}

void LocalMapping::RequestReset()
{
    {
        std::unique_lock<std::mutex> lock(mMutexReset);
        mbResetRequested = true;
    }

    while (1) {
        {
            std::unique_lock<std::mutex> lock2(mMutexReset);
            if (!mbResetRequested) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(3000));
    }
}


void LocalMapping::InsertKeyFrame(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lock(mMutexNewKFs);
    mlNewKeyFrames.push_back(pKF);
    mbAbortBA = true;   // 放弃 Bundle Adjustment
}


bool LocalMapping::stopRequested()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    return mbStopRequested;
}


bool LocalMapping::AcceptKeyFrames()
{
    std::unique_lock<std::mutex> lock(mMutexAccept);
    return mbAcceptKeyFrames;
}


bool LocalMapping::SetNotStop(bool flag)
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    if (flag && mbStopped) {
        return false;
    }

    mbNotStop = flag;

    return true;
}


void LocalMapping::InterruptBA()
{
    mbAbortBA = true;
}

}