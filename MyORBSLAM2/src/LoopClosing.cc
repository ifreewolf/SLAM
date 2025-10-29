#include "LoopClosing.h"

namespace ORB_SLAM2
{

LoopClosing::LoopClosing(Map* pMap, KeyFrameDatabase* pDB, ORBVocabulary* pVoc, const bool bFixScale)
{

}


void LoopClosing::SetTracker(Tracking* pTracker)
{
    mpTracker = pTracker;
}

void LoopClosing::SetLocalMapper(LocalMapping* pLocalMapper)
{
    mpLocalMapper = pLocalMapper;
}


void LoopClosing::Run()
{
    mbFinished = false;
    while (1) {
        static int i = 0;
        // std::cout << "LoopClosing i = " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        i += 1;
    }
}


void LoopClosing::RequestReset()
{
    {
        std::unique_lock<std::mutex> lock(mMutexReset);
        mbResetRequested = true;
    }

    while (1)
    {
        {
            std::unique_lock<std::mutex> lock2(mMutexReset);
            if (!mbResetRequested) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(5000));
    }    
}



}