#include "LoopClosing.h"

namespace ORB_SLAM2
{

LoopClosing::LoopClosing(Map* pMap, KeyFrameDatabase* pDB, ORBVocabulary* pVoc, const bool bFixScale):
    mbResetRequested(false),
    mbFinishRequested(false),
    mbFinished(true),
    mpMap(pMap),
    mpKeyFrameDB(pDB),
    mpORBVocabulary(pVoc),
    mpMatchedKF(NULL),
    mLastLoopKFid(0),
    mbRunningGBA(false),
    mbFinishedGBA(true),
    mbStopGBA(false),
    mpThreadGBA(NULL),
    mbFixScale(bFixScale),
    mnFullBAIdx(0)
{
    mnCovisibilityConsistencyTh = 3;
}


void LoopClosing::SetTracker(Tracking* pTracker)
{
    mpTracker = pTracker;
}

void LoopClosing::SetLocalMapper(LocalMapping* pLocalMapper)
{
    mpLocalMapper = pLocalMapper;
}


/**
 * @brief 回环线程主函数
 * 步骤：
 * 1. 查看闭环检测队列 mlpLoopKeyFrameQueue 中有没有关键帧进来。mlpLoopKeyFrameQueue的关键帧插入在局部建图线程LocalMapping::Run()函数中的mlpLoopCloser->InsertKeyFrame(mpCurrentKeyFrame)代码段实现
 * 2. 如果有关键帧进行，则进行闭环检测，DetectLoop()
 * 3. 如果闭环检测成功，则调用ComputeSim3()计算Sim3
 * 4. 执行循环融合和位姿图优化
 * 5. 查看是否有外部请求复位当前线程，查看外部线程是否有终止当前线程的请求，如果有的话就跳出这个线程的主函数的主循环。
 */
void LoopClosing::Run()
{
    mbFinished = false; // 线程是否完成的标识

    // 线程主循环
    while (1) {
        // 判断是否接收到关键帧
        // Check if there are keyframes in the queue
        // LoopClosing中的关键帧是LocalMapping发送过来的，LocalMapping是Tracking中发送过来的
        // 在LocalMapping中通过 InsertKeyFrame 将关键帧插入闭环检测队列 mlpLoopKeyFrameQueue
        // Step1 查看闭环检测队列 mlpLoopKeyFrameQueue 中有没有关键帧进来
        if (CheckNewKeyFrames()) {
            // Detect loop candidates and check covisibility consistency
            // Step2 如果有关键帧，则进行闭环检测
            if (DetectLoop()) {
                // Step3 如果闭环检测成功，则计算Sim3
                // Compute similarity transormation [sR|t]
                // In the stereo/RGBD case s = 1
                if (ComputeSim3()) {
                    // Step4 执行循环融合和位姿图优化
                    CorrectLoop();
                }
            }
        }

        // Step5 查看是否有外部线程请求复位当前线程
        ResetIfRequested();
        // 查看外部线程是否有终止当前线程的请求，如果有的话就跳出这个线程的主函数的主循环
        if (CheckFinish()) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(5000));
    }

    // 运行到这里说明有外部线程请求终止当前线程，在这个函数中执行终止当前线程的一些操作
    SetFinish();
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


void LoopClosing::ResetIfRequested()
{
    std::unique_lock<std::mutex> lock(mMutexReset);
    if (mbResetRequested) {
        mlpLoopKeyFrameQueue.clear();
        mLastLoopKFid = 0;
        mbResetRequested = false;
    }
}


void LoopClosing::RunGlobalBundleAdjustment(unsigned long nLoopKF)
{
    std::cout << "Starting Global Bundle Adjustment" << std::endl;


}


void LoopClosing::RequestFinish()
{
    std::unique_lock<std::mutex> lock(mMutexFinish);
    mbFinishRequested = true;
}


bool LoopClosing::CheckFinish()
{
    std::unique_lock<std::mutex> lock(mMutexFinish);
    return mbFinishRequested;
}


void LoopClosing::SetFinish()
{
    std::unique_lock<std::mutex> lock(mMutexFinish);
    mbFinished = true;
}


bool LoopClosing::isFinished()
{
    std::unique_lock<std::mutex> lock(mMutexFinish);
    return mbFinished;
}


void LoopClosing::InsertKeyFrame(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lock(mMutexLoopQueue);
    if (pKF->mnId != 0) {
        mlpLoopKeyFrameQueue.emplace_back(pKF);
    }
}


bool LoopClosing::CheckNewKeyFrames()
{
    std::unique_lock<std::mutex> lock(mMutexLoopQueue);
    return (!mlpLoopKeyFrameQueue.empty());
}


/**
 * @brief 挑选出闭环候选关键帧
 * 步骤：
 * 1. 从闭环检测关键帧队列中取出一个关键帧，作为当前闭环检测的关键帧，也对应于源码中的 mpCurrentKF
 * 2. 如果距离上次闭环没多久(小于10帧)，或者map中关键帧总共还没有10帧，则不进行闭环检测
 * 3. 获得所有与当前关键帧连接的关键帧(15个以上共视地图点)，计算当前关键帧与每个共视关键帧的BoW相似度得分，并得到最低得分minScore
 *      这里是一个参考值，后续挑选出来的闭环候选关键帧与当前关键帧的相似度需要大于该值，简单的说，认为和当前关键帧具有回环关系的关键帧，不应该低于
 *      当前关键帧的相邻关键帧的最低的相似度minScore
 * 4. 在所有关键帧中找出闭环候选帧（注意不和当前帧连接），候选关键帧与当前关键帧具有较多的公共单词，相似度都高于minScore。挑选出来的初始闭环候选帧
 *      存储与 vpCandidateKFs 中，如果 vpCandidateKFs为空，则返回false，表示没有检测到闭环。
 */
bool LoopClosing::DetectLoop()
{
    // Step1 从队列中取出一个关键帧，作为当前检测闭环关键帧
    {
        std::unique_lock<std::mutex> lock(mMutexLoopQueue);
        // 从队列头开始取，也就是先取早进来的关键帧
        mpCurrentKF = mlpLoopKeyFrameQueue.front();
        // 取出关键帧之后从队列中弹出该关键帧
        mlpLoopKeyFrameQueue.pop_front();
        // Avoid that a keyframe can be erased while it is being process by this thread
        // 设置当前关键帧不要在优化的过程中被删除
        mpCurrentKF->SetNotErase();
    }

    // Step2 如果距离上次闭环没多久（小于10帧），或者Map中关键帧总共还没有10帧，则不进行闭环检测
    // 后者的体现是当 mLastLoopKFid 为0的时候
    if (mpCurrentKF->mnId < mLastLoopKFid + 10) {
        mpKeyFrameDB->add(mpCurrentKF); // mpKeyFrameDB 是关键帧数据库，所有的关键帧
        mpCurrentKF->SetErase();        // 
        return false;
    }

    // Step3 遍历当前回环关键帧所有连接(>15个共视地图点)，计算当前关键帧与每个共视关键帧的BoW相似度得分，并且得到最低得分minScore
    const std::vector<KeyFrame*> vpConnectedKeyFrames = mpCurrentKF->GetVectorCovisibleKeyFrames();
    const DBoW2::BowVector &CurrentBowVec = mpCurrentKF->mBowVec;
    float minScore = -1;
    for (size_t i = 0; i < vpConnectedKeyFrames.size(); i++) {
        KeyFrame* pKF = vpConnectedKeyFrames[i];
        if (pKF->isBad()) {
            continue;
        }
        const DBoW2::BowVector &BowVec = pKF->mBowVec;
        // 计算两个关键帧的相似度得分：得分越低，相似度越低
        float score = mpORBVocabulary->score(CurrentBowVec, BowVec);
        // 更新最低得分
        if (score < minScore) {
            minScore = score;
        }
    }

    // Step4 在所有关键帧中找出闭环候选帧（注意不和当前帧连接）
    // minScore的作用：认为和当前关键帧具有回环关系的关键帧，不应该低于当前关键帧的相邻关键帧的最低的相似度minScore
    // 得到的这些关键帧，和当前关键帧具有较多的公共单词，并且相似度评分都挺高的
    std::vector<KeyFrame*> vpCandidateKFs = mpKeyFrameDB->DetectLoopCandidates(mpCurrentKF, minScore);

    // 如果没有闭环候选帧，返回false
    if (vpCandidateKFs.empty()) {
        mpKeyFrameDB->add(mpCurrentKF);
        mvConsistentGroups.clear();
        mpCurrentKF->SetErase();
        return false;
    }

    mvpEnoughConsistentCandidates.clear();

    std::vector<ConsistentGroup> vCurrentConsistentGroups;

    // 这个下标是每个“子连续组”的下标，bool表示当前的候选组中是否有和该组相同的一个关键帧
    std::vector<bool> vbConsistentGroup(mvConsistentGroups.size(), false);

    // Step5.1 遍历刚才得到的每一个候选关键帧
    for (size_t i = 0, iend = vpCandidateKFs.size(); i < iend; i++) {
        KeyFrame* pCandidateKF = vpCandidateKFs[i];

        // Step5.2 将自己以及与自己相连的关键帧构成一个“子候选组”
        std::set<KeyFrame*> spCandidateGroup = pCandidateKF->GetConnectedKeyFrames();
        // 把自己也加进去
        spCandidateGroup.insert(pCandidateKF);

        // 连续性达标的标志
        bool bEnoughConsistent = false;
        bool bConsistentForSomeGroup = false;

        // Step5.3 遍历前一次闭环检测到的连续组链
        for (size_t iG = 0, iendG = mvConsistentGroups.size(); iG < iendG; iG++) {
            // 取出之前的一个子连续组中的关键帧集合
            std::set<KeyFrame*> sPreviousGroup = mvConsistentGroups[iG].first;
        }
    }

}

}