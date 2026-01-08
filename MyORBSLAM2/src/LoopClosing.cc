#include "LoopClosing.h"

#include "Optimizer.h"

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

    // Step5 在候选帧中检测具有连续性的候选帧
    // 1. 每个候选帧与自己相连的关键帧构成一个“子候选组spCandidateGroup”，vpCandidateKFs-->spCandidateGroup
    // 2. 检测“子候选组”中每一个关键帧是否存在于“连续组”，如果存在 nCurrentConsistency++，则将该“子候选组”放入“当前连续组vCurrentConsistentGroups”
    // 3. 如果nCurrentConsistency大于等于3，那么该“子候选组”代表的候选帧过关，进入mvpEnoughConsistentCandidates

    // 最终筛选后得到的闭环帧
    mvpEnoughConsistentCandidates.clear();

    // ConsistentGroup数据类型为std::pair<std::set<KeyFrame*>, int>
    // first对应每个“连续组”中的关键帧，second为每个“连续组”的已连续几个的序号

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
        // 上一次闭环的连续组链 std::vector<ConsistentGroup> mvConsistentGroups
        // 其中ConsistentGroup的定义：typedef std::pair<std::set<KeyFrame*>, int> ConsistentGroup
        // 其中 ConsistentGroup.first 对应每个“连续组”中的关键帧集合，ConsistentGroup.second为每个“连续组”的连续长度
        for (size_t iG = 0, iendG = mvConsistentGroups.size(); iG < iendG; iG++) {
            // 取出之前的一个子连续组中的关键帧集合
            std::set<KeyFrame*> sPreviousGroup = mvConsistentGroups[iG].first;

            // Step5.4 遍历每个“子候选组”，检测子候选组中每一个关键帧在“子连续组”中是否存在
            // 如果有一帧共同存在于“子候选组”与之前的“子连续组”，那么“子候选组”与该“子连续组”连续
            bool bConsistent = false;
            for (std::set<KeyFrame*>::iterator sit = spCandidateGroup.begin(), send = spCandidateGroup.end(); sit != send; sit++) {
                if (sPreviousGroup.count(*sit)) {
                    // 如果存在，该“子候选组”与该“子连续组”相连
                    bConsistent = true;
                    // 该“子候选组”至少与一个“子连续组”相连，跳出循环
                    bConsistentForSomeGroup = true;
                    break;
                }
            }

            if (bConsistent) {
                // Step5.5 如果判定为连续，接下来判断是否达到连续的条件
                // 取出和当前的候选组发生“连续”关系的子连续组的“已连续次数”
                int nPreviousConsistency = mvConsistentGroups[iG].second;
                // 将当前候选组连续长度在原子连续组的基础上+1
                int nCurrentConsistency = nPreviousConsistency + 1;
                // 如果上述连续还未记录到 vCurrentConsistentGroups，那么记录一下
                // 注意这里 spCandidateGroup 可能放置在 vbConsistentGroup 中其他索引(iG)下
                if (!vbConsistentGroup[iG]) {
                    // 将该“子候选组”的该关键帧打上连续编号加入到“当前连续组”
                    ConsistentGroup cg = std::make_pair(spCandidateGroup, nCurrentConsistency);
                    // 放入本次闭环检测的连续组 vCurrentConsistentGroups 里
                    vCurrentConsistentGroups.push_back(cg);
                    // 标记一下，防止重复添加到同一个索引iG
                    // 但是 spCandidateGroup 可能重复添加到不同的索引iG对应的 vbConsistentGroup 中
                    vbConsistentGroup[iG] = true;
                }

                // 如果连续长度满足要求，那么当前的这个候选关键帧是足够靠谱的
                // 连续性阈值 mnCovisibilityConsistencyTh = 3;
                // 足够连续的标记 bEnoughConsistent
                if (nCurrentConsistency >= mnCovisibilityConsistencyTh && !bEnoughConsistent) {
                    // 记录为达到连续条件了
                    mvpEnoughConsistentCandidates.push_back(pCandidateKF);
                    // 标记一下，防止重复添加
                    bEnoughConsistent = true;

                    // ? 这里可以break掉结束当前for循环吗？
                    // 回答：不行。因为虽然pCandidateKF达到了连续性要求
                    // 但 spCandidateGroup 还可以和 mvConsistentGroups 中其他的子连续组进行连接
                }
            }
        }

        // Step5.6 如果该“子候选组”的所有关键帧和上次闭环无关（不连续），vCurrentConsistentGroups 没有新添加连续关系
        // 于是就把“子候选组”全部拷贝到 vCurrentConsistentGroups，用于更新mvConsistentGroups，连续性计数器设为0
        if (!bConsistentForSomeGroup) {
            ConsistentGroup cg = std::make_pair(spCandidateGroup, 0);
            vCurrentConsistentGroups.push_back(cg);
        }
    }   // 遍历得到的初级候选关键帧

    // 更新连续组
    mvConsistentGroups = vCurrentConsistentGroups;

    // 当前闭环检测的关键帧添加到关键帧数据库中
    mpKeyFrameDB->add(mpCurrentKF);

    if (mvpEnoughConsistentCandidates.empty()) {
        // 未检测到闭环，返回false
        mpCurrentKF->SetErase();
        return false;
    } else {
        // 成功检测到闭环，返回true
        return true;
    }

    // 多余的代码，执行不到
    mpCurrentKF->SetErase();
    return false;
}


/**
 * @brief 计算当前关键帧和上一步闭环候选帧的Sim3变换
 * 1. 遍历闭环候选帧集，筛选出与当前帧的匹配特征点数大于20的候选帧集合，并未每一个候选帧构造一个Sim3Solver
 * 2. 对每一个候选帧进行 Sim3Solver 迭代匹配，直到有一个候选匹配成功，或者全部失败
 * 3. 取出闭环匹配上关键帧的相连关键帧，得到它们的地图点放入 mvpLoopMapPoints
 * 4. 将闭环匹配上关键帧以及相连关键帧的地图点投影到当前关键帧进行投影匹配
 * 5. 判断当前帧与检测出的所有闭环关键帧是否有足够多的地图点匹配
 * 6. 清空 mvpEnoughConsistentCandidates
 * @return true     只要有一个候选关键帧通过Sim3的求解与优化，就返回true
 * @return false    所有候选关键帧与当前关键帧都没有有效Sim3变换
 */
bool LoopClosing::ComputeSim3()
{
    // Sim3 计算流程
    // 1. 通过BoW加速描述子的匹配，利用RANSAC粗略地计算出当前帧与闭环帧的Sim3 (当前帧---闭环帧)
    // 2. 根据估计的Sim3，对3D点进行投影找到更多匹配，通过优化的方法计算更精确的Sim3（当前帧---闭环帧）
    // 3. 将闭环帧以及闭环帧相连的关键帧的地图点与当前帧的地图点进行匹配（当前帧---闭环帧+相连关键帧）
    // 注意以上匹配的结果均存在成员变量 mvpCurrentMatchedPoints 中，实际的更新步骤见 CorrectLoop() 步骤3
    // 对于双目或者是RGBD输入的情况，计算得到的尺度=1

    // 准备工作
    // 对每个闭环候选帧都准备一个Sim3Solver
    const int nInitialCandidates = mvpEnoughConsistentCandidates.size();

    // 首先为每个候选帧计算ORB特征匹配，当找到足够数量的匹配点后，就会建立一个Sim3求解器
    ORBmatcher matcher(0.75, true); // 0.75 是最优评分和次优评分比例的阈值，true表示对特征点的方向进行检查，也就是进行角度差筛选

    // 存储每一个候选帧的SimSolver求解器
    std::vector<Sim3Solver*> vpSim3Solvers;
    vpSim3Solvers.resize(nInitialCandidates);

    // 存储每个候选帧的匹配地图点信息
    std::vector<std::vector<MapPoint*>> vvpMapPointMatches;
    vvpMapPointMatches.resize(nInitialCandidates);

    // 存储每个候选帧应该被放弃(True)或者保留(False)
    std::vector<bool> vbDiscarded;
    vbDiscarded.resize(nInitialCandidates);

    // 完成Step1的匹配后，被保留的候选帧数量
    int nCandidates = 0;    // candidates with enough matches

    // Step1 遍历闭环候选帧，初步筛选出与当前关键帧的匹配特征点数大于20的候选帧集合，并为每一个候选帧构造一个Sim3Solver
    for (int i = 0; i < nInitialCandidates; i++) {
        // Step1.1 从筛选的闭环帧中取出一帧有效关键帧pKF
        KeyFrame* pKF = mvpEnoughConsistentCandidates[i];

        // 避免再LocalMapping中KeyFrameCulling函数将此关键帧作为冗余帧剔除
        pKF->SetNotErase();

        // 如果候选帧质量不高，直接PASS
        if (pKF->isBad()) {
            vbDiscarded[i] = true;
            continue;
        }

        // Step1.2 将当前帧 mpCurrentKF 与闭环候选关键帧pKF匹配
        // 通过BoW加速得到 mpCurrentKF 与 pKF之间的匹配特征点
        // vvpMapPointMatches 是匹配特征点对应的地图点，本质上来自于候选闭环帧
        int nmatches = matcher.SearchByBoW(mpCurrentKF, pKF, vvpMapPointMatches[i]);

        // 粗筛：匹配的特征点数太少，该候选帧剔除
        if (nmatches < 20) {
            vbDiscarded[i] = true;
            continue;
        } else {
            // Step1.3 为保留的候选帧构造Sim3求解器
            // 如果 mbFixScale（是否固定尺寸）为true，则是 6 自由度优化(双目RGBD)
            // 如果是false，则是7自由度优化(单目)
            Sim3Solver* pSolver = new Sim3Solver(mpCurrentKF, pKF, vvpMapPointMatches[i], mbFixScale);

            // Sim3Solver RNASAC过程置信度0.99，至少20个inliers，最多迭代300次迭代
            pSolver->SetRansacParameters(0.99, 20, 300);
            vpSim3Solvers[i] = pSolver;
        }

        // 保留的候选帧数量
        nCandidates++;
    }

    // 用于标记是否有一个候选帧通过Sim3Solver的求解与优化
    bool bMatch = false;

    // Step2 对每一个候选帧用Sim3Solver迭代匹配， 直到有一个候选帧匹配成功，或者全部失败
    while (nCandidates > 0 && !bMatch) {
        // 遍历每一个候选帧
        for (int i = 0; i < nInitialCandidates; i++) {
            if (vbDiscarded[i]) {
                continue;
            }

            KeyFrame* pKF = mvpEnoughConsistentCandidates[i];

            // 内点（Inliers）标志
            // 即标记经过RANSAC Sim3求解后，vvpMapPointMatches中的哪些作为内点
            std::vector<bool> vbInliers;

            // 内点（Inliers）数量
            int nInliers;

            // 是否达到了最优解
            bool bNoMore;

            // Step2.1 取出从 Step1.3 中为当前候选帧构建的 Sim3Solver 并开始迭代
            Sim3Solver* pSolver = vpSim3Solvers[i];

            // Sim3计算匹配，最多迭代5次，返回的Scm是候选帧pKF到当前帧 mpCurrentKF 的Sim3变换(T12)
            cv::Mat Scm = pSolver->iterate(5, bNoMore, vbInliers, nInliers);

            // 总迭代次数达到最大限制还没有求出合格的Sim3变换，该候选帧剔除
            if (bNoMore) {
                vbDiscarded[i] = true;
                nCandidates--;
            }

            // 如果计算出了Sim3变换，继续匹配出更多点并优化。因为之前 SearchByBoW 匹配可能会有遗漏
            if (!Scm.empty()) {
                // 取出经过 Sim3Solver 后匹配点中的内点集合
                std::vector<MapPoint*> vpMapPointMatches(vvpMapPointMatches[i].size(), static_cast<MapPoint*>(NULL));
                for (size_t j = 0, jend = vbInliers.size(); j < jend; j++) {
                    // 保存内点
                    if (vbInliers[j]) {
                        vpMapPointMatches[j] = vvpMapPointMatches[i][j];
                    }
                }

                // Step2.2 通过上面求得Sim3变换引导关键帧匹配，弥补Step1中的漏匹配
                // 候选帧pKF到当前帧 mpCurrentKF 的 R(R12),t(T12)，变换尺度s(s12)
                cv::Mat R = pSolver->GetEstimatedRotation();
                cv::Mat t = pSolver->GetEstimatedTranslation();
                const float s = pSolver->GetEstimatedScale();

                matcher.SearchBySim3(mpCurrentKF, pKF, vpMapPointMatches, s, R, t, 7.5);

                // Step2.3 用新的匹配来优化Sim3，只要有一个候选帧通过Sim3的求解与优化，就跳出停止对其它候选帧的判断
                // OpenCV的Mat矩阵转成Eigen的Matrix类型
                // gScm：候选关键帧到当前帧的Sim3变换
                g2o::Sim3 gScm(Converter::toMatrix3d(R), Converter::toVector3d(t), s);

                // 如果mbFixScale为true，则是6自由度优化(双目/RGBD)，如果是false，则是7自由度优化(单目)
                // 优化 mpCurrentKF 与 pKF对应的MapPoints间的Sim3，得到优化后的量 gScm
                const int nInliers = Optimizer::OptimizeSim3(mpCurrentKF, pKF, vpMapPointMatches, gScm, 10, mbFixScale);

                // 如果优化成功，则停止 while 循环遍历闭环候选帧
                if (nInliers >= 20) {
                    bMatch = true;
                    // mpMatchedKF 就是最终闭环检测出来与当前帧形成闭环的关键帧
                    mpMatchedKF = pKF;

                    // gSmw:从世界坐标系 w 到该候选帧 m 的Sim3变换，都在一个坐标系下，所以尺度 scale = 1
                    g2o::Sim3 gSmw(Converter::toMatrix3d(pKF->GetRotation()), Converter::toVector3d(pKF->GetTranslation()), 1.0);

                    // 得到g2o优化后从世界坐标系到当前帧的Sim3变换
                    // gScm：候选帧到当前帧的Sim3变换
                    // 所以mg2oScw：就是世界坐标系到当前帧的Sim3变换
                    mg2oScw = gScm * gSmw;
                    mScw = Converter::toCvMat(mg2oScw);
                    mvpCurrentMatchedPoints = vpMapPointMatches;

                    // 只要有一个候选帧通过Sim3的求解与优化，就跳出停止对其它候选帧的判断
                    break;
                }

            }

        }
    }

    // 退出上面while循环的原因有两种：一种是求解到了bMatch置位后出的，另外一种是 nCandidates 耗尽为0
    if (!bMatch) {
        // 如果没有一个闭环匹配候选帧通过Sim3的求解与优化
        // 清空 mvpEnoughConsistentCandidates，这些候选关键帧以后都不会再参加回环检测过程了
        for (int i = 0; i < nInitialCandidates; i++) {
            mvpEnoughConsistentCandidates[i]->SetErase();
        }
        // 当前关键帧也将不会再参加回环检测了
        mpCurrentKF->SetErase();
        // Sim3 计算失败，退出了
        return false;
    }

    // Step3 取出与当前帧闭环匹配上的关键帧及其共视关键帧，以及这些共视关键帧的地图点
    // 注意是闭环检测出来与当前帧形成闭环的关键帧 mpMatchedKF
    // 将 mpMatchedKF 共视的关键帧全部取出来放入 vpLoopConnectedKFs
    // 将 vpLoopConnectedKFs 的地图点取出来放入 mvpLoopMapPoints
    std::vector<KeyFrame*> vpLoopConnectedKFs = mpMatchedKF->GetVectorCovisibleKeyFrames();

    // 包含闭环匹配关键帧本身，形成一个“闭环关键帧小组”
    vpLoopConnectedKFs.push_back(mpMatchedKF);
    mvpLoopMapPoints.clear();

    // 遍历这个组中的每一个关键帧
    for (std::vector<KeyFrame*>::iterator vit = vpLoopConnectedKFs.begin(); vit != vpLoopConnectedKFs.end(); vit++) {
        KeyFrame* pKF = *vit;
        std::vector<MapPoint*> vpMapPoints = pKF->GetMapPointMatches();

        // 遍历关键帧的所有有效地图点
        for (size_t i = 0, iend = vpMapPoints.size(); i < iend; i++) {
            MapPoint* pMP = vpMapPoints[i];
            if (pMP) {
                // mnLoopPointForKF 用于标记，避免重复添加
                if (!pMP->isBad() && pMP->mnLoopPointForKF != mpCurrentKF->mnId) {
                    mvpLoopMapPoints.push_back(pMP);
                    // 标记一下
                    pMP->mnLoopPointForKF = mpCurrentKF->mnId;
                }
            }
        }
    }

    // Step4 将闭环关键帧及其连接关键帧的所有地图点投影到当前关键帧进行投影匹配
    // 根据投影查找更多的匹配（成功的闭环匹配需要满足足够多的匹配特征点数）
    // 根据Sim3变换，将每个 mvpLoopMapPoints 投影到 mpCurrentKF 上，搜索新的匹配对
    // mvpCurrentMatchedPoints 是前面经过 SearchBySim3 得到的已经匹配的点对，这里就忽略不再匹配了
    // 搜索范围系数为10
    matcher.SearchByProjection(mpCurrentKF, mScw, mvpLoopMapPoints, mvpCurrentMatchedPoints, 10);

    // Step5：统计当前帧与闭环关键帧的匹配地图点数目，超过40个说明成功闭环，否则失败
    int nTotalMatches = 0;
    for (size_t i = 0; i < mvpCurrentMatchedPoints.size(); i++) {
        if (mvpCurrentMatchedPoints[i]) {
            nTotalMatches++;
        }
    }

    if (nTotalMatches >= 40) {
        // 如果当前回环可靠，保留当前待闭环关键帧，其它闭环候选全部删掉以后不用了
        for (int i = 0; i < nInitialCandidates; i++) {
            if (mvpEnoughConsistentCandidates[i] != mpMatchedKF) {
                mvpEnoughConsistentCandidates[i]->SetErase();
            }
        }
        return true;
    } else {
        // 闭环不可靠，闭环候选及当前待闭环帧全部删除
        for (int i = 0; i < nInitialCandidates; i++) {
            mvpEnoughConsistentCandidates[i]->SetErase();
        }
        mpCurrentKF->SetErase();
        return false;
    }
}


/**
 * @brief 闭环矫正
 * 1. 通过求解的Sim3以及相对位姿关系，调整与当前帧相连的关键帧位姿以及这些关键帧观测到的地图点位置（相连关键帧 --- 当前帧）
 * 2. 将闭环帧以及闭环帧相连的关键帧的地图点和与当前帧相连的关键帧相连的关键帧的点进行匹配（当前帧+相连关键帧 --- 闭环帧+相连关键帧）
 * 3. 通过MapPoints的匹配关系更新这些帧之间的连接关系，即更新Covisibility graph
 * 4. 对Essential Graph（Pose Graph）进行优化，MapPoints的位置则根据优化后的位姿做相对应的调整
 * 5. 创建线程进行全局Bundle Adjustment
 */
void LoopClosing::CorrectLoop()
{
    // Step0 结束局部地图线程、全局BA，为闭环矫正做准备
    // 请求局部地图停止，防止在回环矫正时局部地图线程中InsertKeyFrame函数插入新的关键帧
    mpLocalMapper->RequestStop();

    if (isRunningGBA()) {   // 如果闭环线程正在运行Global Bundle Adjustment，终止它，因为要进行新的全局BA
        std::unique_lock<std::mutex> lock(mMutexGBA);
        mbStopGBA = true;
        // 记录全局BA次数
        mnFullBAIdx++;
        if (mpThreadGBA) {
            // 停止全局BA线程
            mpThreadGBA->detach();
            delete mpThreadGBA;
        }
    }

    // 一直等到局部地图线程结束再继续
    while (!mpLocalMapper->isStopped()) {
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }

    // Step1 根据共视关系更新当前关键帧与其它关键帧之间的连接关系
    // 因为之前闭环检测、计算Sim3中改变了该关键帧的地图点，所以需要更新
    mpCurrentKF->UpdateConnections();

    // Step2 通过位姿传播，得到Sim3优化后，与当前帧相连的关键帧的位姿，以及它们的地图点
    // 当前帧与世界坐标系之间的Sim变换在ComputeSim3函数中已经确定并优化
    // 通过相对位姿关系，可以确定这些相连的关键帧与世界坐标系之间的Sim3变换

    // 取出当前关键帧及其共视关键帧，称为“当前关键帧组”
    mvpCurrentConnectedKFs = mpCurrentKF->GetVectorCovisibleKeyFrames();
    mvpCurrentConnectedKFs.push_back(mpCurrentKF);

    // CorrectedSim3：存放闭环g2o优化后当前关键帧的共视关键帧的世界坐标系下 Sim3 变换
    // NonCorrectedSim3：存放没有矫正的当前关键帧的共视关键帧的世界坐标系下 Sim3 变换
    KeyFrameAndPose CorrectedSim3, NonCorrectedSim3;
    // 先将mpCurrentKF的Sim3变换存入，认为是准的，所以固定不变
    CorrectedSim3[mpCurrentKF] = mg2oScw;
    // 当前关键帧到世界坐标系下的变换矩阵
    cv::Mat Twc = mpCurrentKF->GetPoseInverse();
    
    // 对地图点操作
    {
        // 锁定地图点
        std::unique_lock<std::mutex> lock(mpMap->mMutexMapUpdate);

        // Step2.1 通过 mg2oScw(认为是准的)来进行位姿传播，得到当前关键帧的共视关键帧的世界坐标系下 Sim3 位姿
        // 遍历“当前关键帧组”
        for (std::vector<KeyFrame*>::iterator vit = mvpCurrentConnectedKFs.begin(), vend = mvpCurrentConnectedKFs.end(); vit != vend; vit++) {
            KeyFrame* pKFi = *vit;
            cv::Mat Tiw = pKFi->GetPose();  // 世界坐标系到该帧相机坐标系的变换

            if (pKFi != mpCurrentKF) {  // 跳过当前关键帧，因为当前关键帧的位姿已经在前面优化过了，在这里是参考基准
                cv::Mat Tic = Tiw * Twc;    // 当前帧到相连帧i的位姿
                cv::Mat Ric = Tic.rowRange(0, 3).colRange(0, 3);
                cv::Mat tic = Tic.rowRange(0, 3).col(3);

                // g2oSic：当前关键帧 mpCurrentKF 到其共视关键帧pKFi的Sim3相对变换
                // 这里是non-correct，所以scale=1.0
                g2o::Sim3 g2oSic(Converter::toMatrix3d(Ric), Converter::toVector3d(tic), 1.0);
                // 当前帧的位姿固定不动，其它的关键帧根据相对关系得到 Sim3 调整的位姿。依据是：当前帧与共视帧之间的相对位姿不变
                g2o::Sim3 g2oCorrectedSiw = g2oSic * mg2oScw;
                // 存放闭环g2o优化后当前关键帧的共视关键帧的Sim3位姿
                CorrectedSim3[pKFi] = g2oCorrectedSiw;
            }

            cv::Mat Riw = Tiw.rowRange(0, 3).colRange(0, 3);
            cv::Mat tiw = Tiw.rowRange(0, 3).col(3);
            g2o::Sim3 g2oSiw(Converter::toMatrix3d(Riw), Converter::toVector3d(tiw), 1.0);
            // 存放没有矫正的当前关键帧的共视关键帧的 Sim3 变换
            NonCorrectedSim3[pKFi] = g2oSiw;
        }

        // Step2.2 得到矫正的当前关键帧的共视关键帧位姿后，修正这些共视关键帧的地图点
        // 遍历待矫正的共视关键帧（不包括当前关键帧）
        for (KeyFrameAndPose::iterator mit = CorrectedSim3.begin(), mend = CorrectedSim3.end(); mit != mend; mit++) {
            // 取出当前关键帧连接关键帧
            KeyFrame* pKFi = mit->first;
            // 取出经过位姿传播后的 Sim3 变换
            g2o::Sim3 g2oCorrectedSiw = mit->second;
            g2o::Sim3 g2oCorrectedSwi = g2oCorrectedSiw.inverse();
            // 取出未经位姿传播的Sim3变换
            g2o::Sim3 g2oSiw = NonCorrectedSim3[pKFi];

            std::vector<MapPoint*> vpMPsi = pKFi->GetMapPointMatches();

            // 遍历待矫正共视关键帧中的每一个地图点
            for (size_t iMP = 0, endMPi = vpMPsi.size(); iMP < endMPi; iMP++) {
                MapPoint* pMPi = vpMPsi[iMP];
                // 跳过无效的地图点
                if (!pMPi) {
                    continue;
                }
                if (!pMPi->isBad()) {
                    continue;
                }
                // 标记，防止重复矫正
                if (pMPi->mnCorrectedByKF == mpCurrentKF->mnId) {
                    continue;
                }

                // 矫正过程本质上也是基于当前关键帧优化后的位姿展开的
                // 将该未矫正的eigP2Dw先从世界坐标系系映射到未矫正的pKFi相机坐标系，然后再反映射到矫正后的世界坐标系下
                cv::Mat P3Dw = pMPi->GetWorldPos();
                // 地图点在世界坐标系下坐标
                Eigen::Matrix<double, 3, 1> eigP3Dw = Converter::toVector3d(P3Dw);
                // map(p)内部做了相似变换 s*R*P + t
                // 下面变换是：eigP2Dw：world --> g2oSiw --> i --> g2oCorrectedSwi --> world
                Eigen::Matrix<double, 3, 1> eigCorrectedP3Dw = g2oCorrectedSwi.map(g2oSiw.map(eigP3Dw));    // g2oCorrectedSwi：相连关键帧到世界坐标系的位姿变换

                cv::Mat cvCorrectedP3Dw = Converter::toCvMat(eigCorrectedP3Dw);
                pMPi->SetWorldPos(cvCorrectedP3Dw);
                // 记录矫正该地图点的关键帧id，防止重复
                pMPi->mnCorrectedByKF = mpCurrentKF->mnId;
                // 记录该地图点所在的关键帧id
                pMPi->mnCorrectedReference = pKFi->mnId;
                // 因为地图点更新了，需要更新其平均观测方向以及观测距离范围
                pMPi->UpdateNormalAndDepth();
            }

            // Step2.3 将共视关键帧的Sim3转换为SE3，根据更新的Sim3，更新关键帧的位姿
            // 其实是现在已经有了更新后的关键帧组中关键帧的位姿，但是在上面的操作时只是暂时存储到了 KeyFrameAndPose 类型的变量中，还没有写回到关键帧对象中
            // 调用 toRotationMatrix 可以自动归一化旋转矩阵
            Eigen::Matrix3d eigR = g2oCorrectedSiw.rotation().toRotationMatrix();
            Eigen::Vector3d eigt = g2oCorrectedSiw.translation();
            double s = g2oCorrectedSiw.scale();

            // 平移向量中包含有尺度信息，还需要用尺度归一化
            eigt *= (1./s); // [R t/s; 0 1]

            cv::Mat correctedTiw = Converter::toCvSE3(eigR, eigt);
            // 设置矫正后的新的pose
            pKFi->SetPose(correctedTiw);

            // Step2.4 根据共视关系更新当前帧与其它关键帧之间的连接
            // 地图点的位置改变了，可能会引起共视关系/权值的改变
            pKFi->UpdateConnections();
        }

        // Step3 检查当前帧的地图点与经过闭环匹配后该帧的地图点是否存在冲突，对冲突的进行替换或填补
        // mvpCurrentMatchedPoints 是当前关键帧和闭环关键帧组的所有地图点进行投影得到的匹配点
        for (size_t i = 0; i < mvpCurrentMatchedPoints.size(); i++) {
            if (mvpCurrentMatchedPoints[i]) {
                // 取出同一个索引对应的两种地图点，决定是否要替换
                // 匹配投影得到的地图点
                MapPoint* pLoopMP = mvpCurrentMatchedPoints[i];
                // 原来的地图点
                MapPoint* pCurMP = mpCurrentKF->GetMapPoint(i);
                if (pCurMP) {   // 如果有重复的MapPoint，则用匹配的地图点代替现有的；因为匹配的地图点是经过一系列操作后比较精确的，现有的地图点很可能有累计误差
                    pCurMP->Replace(pLoopMP);
                } else {    // 如果当前帧没有该MapPoint，则直接添加
                    mpCurrentKF->AddMapPoint(pLoopMP, i);
                    pLoopMP->AddObservation(mpCurrentKF, i);
                    pLoopMP->ComputeDistinctiveDescriptors();
                }
            }
        }
    }

    // Step4 将闭环相连关键组 mvpLoopMapPoints投影到当前关键帧组中，进行匹配、融合、新增或替换当前关键帧组中KF的地图点
    // 因为闭环相连关键帧组 mvpLoopMapPoints 在地图中时间比较久经历了多次优化，认为是准确的
    // 而当前关键帧组中的关键帧的地图点是最近新计算的，可能有累积误差
    // CorrectedSim3：存放矫正后当前关键帧的共视关键帧，及其世界坐标系下 Sim3 变换
    SearchAndFuse(CorrectedSim3);

    // Step5 更新当前关键帧组之间的两级共视相连关系，得到因闭环时地图点融合而新得到的连接关系
    /**
     * @brief 更新关键帧组之间的两级共视连接关系，得到因闭环时地图点融合而新得到的连接关系。新的连接关系存储于 LoopConnections 变量之中
     * 1. 遍历当前关键帧相连的关键帧组(一级相连),然后获得相连的关键帧的共视关键帧 vpPreviousNeighbors(二级相连);
     * 2. 更新当前关键帧相连关键帧的连接关系,存储于 LoopConnections 之中
     * 3. vpPreviousNeighbors 与 mvpCurrentConnectedKFs 的连接关系,如果与LoopConnections中的连接关系保持一致,说明其没有更新,所以会删除没有更新连接关系的连接帧;
     * 4. 最终LoopConnections表示的,是当前帧相连关键帧组(一级相连),连接关系发生改变的相连关键帧,以及其对应的连接关系.
     */
    // LoopConnections:存储因为闭环地图点调整而新生成的连接关系
    std::map<KeyFrame*, std::set<KeyFrame*>> LoopConnections;

    // Step5.1 遍历当前帧相连关键帧组(一级相连)
    for (std::vector<KeyFrame*>::iterator vit = mvpCurrentConnectedKFs.begin(), vend = mvpCurrentConnectedKFs.end(); vit != vend; vit++) {
        KeyFrame* pKFi = *vit;
        // Step5.2 得到与当前帧相连关键帧的相连关键帧(二级相连)
        std::vector<KeyFrame*> vpPreviousNeighbors = pKFi->GetVectorCovisibleKeyFrames();

        // Step5.3 更新一级相连关键帧的连接关系(会把当前关键帧添加进去,因为地图点已经更新和替换了)
        pKFi->UpdateConnections();
        // Step5.4 取出该帧更新后的连接关系
        LoopConnections[pKFi] = pKFi->GetConnectedKeyFrames();
        // Step5.5 从连接关系中去除闭环之前的二级连接关系,剩下的连接就是由闭环都得到的连接关系
        for (std::vector<KeyFrame*>::iterator vit_prev = vpPreviousNeighbors.begin(), vend_prev = vpPreviousNeighbors.end(); vit_prev != vend_prev; vit_prev++) {
            LoopConnections[pKFi].erase(*vit_prev);
        }

        // Step5.6 从连接关系中去除闭环之前的一级连接关系,剩下的连接就是由闭环得到的连接关系
        for (std::vector<KeyFrame*>::iterator vit2 = mvpCurrentConnectedKFs.begin(), vend2 = mvpCurrentConnectedKFs.end(); vit2 != vend2; vit2++) {
            LoopConnections[pKFi].erase(*vit2);
        }
    }

    // Step6 进行本质图优化,优化本质图中所有关键帧的位姿和地图点
    // LoopConnections是关于当前关键帧的共视关键帧的共视关键帧连接关系,属于当前帧的二级连接关系
    // LoopConnections是形成闭环后新生成的连接关系,不包括当前帧与闭环匹配帧之间的连接关系
    Optimizer::OptimizeEssentialGraph(mpMap, mpMatchedKF, mpCurrentKF, NonCorrectedSim3, CorrectedSim3, LoopConnections, mbFixScale);

    mpMap->InformNewBigChange();

    // Step7 添加当前帧与闭环匹配帧之间的边(这个连接关系不优化)
    mpMatchedKF->AddLoopEdge(mpCurrentKF);
    mpCurrentKF->AddLoopEdge(mpMatchedKF);

    // Step8 新建一个线程用于全部BA优化
    // OptimizeEssentialGraph只是优化了一些主要关键帧的位姿,这里进行全局BA可以全局优化所有位姿和MapPoints
    mbRunningGBA = true;
    mbFinishedGBA = false;
    mbStopGBA = false;
    mpThreadGBA = new std::thread(&LoopClosing::RunGlobalBundleAdjustment, this, mpCurrentKF->mnId);

    // Loop closed. Release Local Mapping.
    mpLocalMapper->Release();

    mLastLoopKFid = mpCurrentKF->mnId;
}


/**
 * @brief 将闭环相连关键帧组 mvpLoopMapPoints 投影到当前关键帧中，进行匹配。新增或替换当前关键帧组中KF的地图点
 * 因为闭环相连关键帧组 mvpLoopMapPoints 在地图中时间比较久经历了多次优化，认为是准确的
 * 而当前关键帧组中的关键帧的地图点是最近新计算的，可能有累计误差
 * 
 * @param[in] CorrectedPoseMap 矫正的当前KF对应的共视关键帧及Sim3变换
 */
void LoopClosing::SearchAndFuse(const KeyFrameAndPose &CorrectedPosesMap)
{
    // 定义ORB匹配器
    ORBmatcher matcher(0.8);

    // Step1 遍历待矫正的当前关键帧的相连关键帧
    for (KeyFrameAndPose::const_iterator mit = CorrectedPosesMap.begin(), mend = CorrectedPosesMap.end(); mit != mend; mit++) {
        KeyFrame* pKF = mit->first; // 当前关键帧的相连关键帧
        // 矫正过的Sim变换
        g2o::Sim3 g2oScw = mit->second;
        cv::Mat cvScw = Converter::toCvMat(g2oScw);

        // Step2 将mvpLoopMapPoints投影到pKF帧匹配，检查地图点冲突并融合
        // mvpLoopMapPoints：与当前关键帧闭环匹配上的关键帧及其共视关键帧组成的地图点
        std::vector<MapPoint*> vpReplacePoints(mvpLoopMapPoints.size(), static_cast<MapPoint*>(NULL));
        // vpReplacePoints：存储mvpLoopMapPoints投影到pKF匹配后需要替换掉的新增地图点，索引和mvpLoopMapPoints一致，初始化为空
        // 搜索区域系数为4
        matcher.Fuse(pKF, cvScw, mvpLoopMapPoints, 4, vpReplacePoints);

        // Get Map Mutx
        // 之所以不在上面Fuse函数中进行地图点融合更新的原因是需要对地图枷锁
        std::unique_lock<std::mutex> lock(mpMap->mMutexMapUpdate);
        const int nLP = mvpLoopMapPoints.size();

        // Step3 遍历闭环帧组成的所有的地图点，替换掉需要替换的地图点
        for (int i = 0; i < nLP; i++) {
            MapPoint* pRep = vpReplacePoints[i];
            if (pRep) {
                // 如果记录了需要替换的地图点
                // 用mvpLoopMapPoints替换掉vpReplacePoints里记录的要替换的地图点
                pRep->Replace(mvpLoopMapPoints[i]); //用mvpLoopMapPoints[i]替换掉pRep
            }
        }   // 将mvpLoopMapPoints中得地图点投影到pKF中，然后再用mvpLoopMapPoints对应位置的地图点去替换
    }

}

}