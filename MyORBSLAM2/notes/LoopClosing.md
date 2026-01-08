# 一、分析`LoopClosing::RequestReset()`方法

## 1.1 `RequestReset`调用

- RequestReset只在一个地方被调用，是在`Tracking::Reset()`中`mpLoopClosing->RequestReset()`

## 1.2 `RequestReset`逻辑

- 1.首先在RequestReset中将mbResetRequested变量置为true，表示请求对线程进行重置；
- 2.RequestReset中，对mbResetRequested变量置为true后，再进入循环，直到检测到mbResetRequested变量被重新置为false，检测到为false
之后就完成了。
- 3.所以mbResetRequested变量会在哪里被置为false，它是一个protected变量，所以仅考虑在LoopClosing内部对它的修改，可以看到是在`ResetIfRequested()`中被置为false的，在置为false之前，会把`mlpLoopKeyFrameQueue`清空，mLastLoopKFid置为0。
- 4.`ResetIfRequested()`是在`LoopClosing`的线程主函数`Run`函数中被调用的，它的调用逻辑是每做一次循环都调用一次，检查是否需要重置线程；
- 5.`ResetIfRequested()`会判断`mbResetRequested`是否为true，也就是在步骤1的`RequestReset`中将其置为true，如果为true，才会改成false，并清理`mlpLoopKeyFrameQueue`。
- 所以，RequestReset中，对mbResetRequested变量置为true后，再进入循环，直到检测到mbResetRequested变量被重新置为false，其实就是在等待主线程调用`ResetIfRequested()`，也就是要清空资源，否则就一直等待。

# 二、分析`DetectLoop`方法

```bash
1. 从队列中取出一个关键帧，作为当前检测闭环关键帧 mpCurrentKF = mlpLoopKeyFrameQueue.front()
2. 遍历当前回环关键帧所有连接，计算当前关键帧与每个共视关键帧的BoW相似度得分，并得到最低得分
    vpConnectedKeyFrames = mpCurrentKF->GetVectorCovisibleKeyFrames()
    &CurrentBowVec = mpCurrentKF->mBowVec
    for (KeyFrame* pkF : vpConnectedKeyFrames)
        &BowVec = pKF->mBowVec;
        float score = mpORBVocabulary->score(CurrentBowVec, BowVec)
        minScore = std::min(score, minScore);
3. 在帧数据库中找出闭环候选帧(不和当前帧连接，使用minScore阈值)
    vpCandidateKFs = mpKeyFrameDB->DetectLoopCandidates(mpCurrentKF, minScore)
4. 在候选帧中检测具有连续性的候选帧
    每个候选帧与自己相连的关键帧构成一个“子候选组 spCandidateGroup”，vpCandidateKFs-->spCandidateGroup
    检测“子候选组”中每一个关键帧是否存在于“连续组”，连续性+1，并将该子候选组放入“当前连续组中 vCurrentConsistentGroups”
    如果nCurrentConsistency大于等于3，那么该“子候选组”代表的候选帧过关，进入 mvpEnoughConsistentCandidates

    上一次闭环的连续组链：mvConsistentGroups
    创建一个关于上一次闭环连续组的bool数组，用于统计上一次闭环连续组的子连续组中是否找到候选帧连续组中相同的关键帧
    std::vector<bool> vbConsistentGroup(mvConsistentGroups.size(), false)
    循环遍历上一步得到的每个候选关键帧
    for (KeyFrame* pCandidateKF : vpCandidateKFs)
        将自己以及与自己相连的关键帧构成一个“子候选组”
        std::set<KeyFrame*> spCandidateGroup = pCandidateKF->GetConnectedKeyFrames()
        连续性达标的标志
        bool bEnoughConsistent = false  # 表示该候选帧满足连续条件
        bool bConsistentForSomeGroup = false    # 表示该候选帧组产生了连续，但不一定满足连续条件
        循环遍历前一次闭环检测到的连续组链
        for (size_t iG : mvConsistentGroups.size())
            取出前一次连续组的子连续组
            std::set<KeyFrame*> sPreviousGroup = mvConsistentGroups[iG].first
            设置布尔变量，用于判断“子候选组”中是否存在连续的关键帧 bool bConsistent = false
            循环遍历"子候选组"
            for (KeyFrame* cKF : spCandidateGroup)
                判断该候选组的关键帧是否包含在前一次连续组的子连续组内
                if (sPreViousGroup.count(cKF)) 
                    如果存在，则表示该“子候选组”是连续的，与该子连续组连续
                    bConsistent = true
                    该“子候选组”至少与一个“子连续组”相连，跳出循环
                    bConsistentForSomeGroup = true
                    跳出循环
                    break;
            判断是否存在连续性
            if (bConsistent)
                取出和当前的候选组发生“连续”关系的子连续组的“已连续次数”
                int nPreviousConsistency = mvConsistentGroups[iG].second
                将当前候选组连续长度在原子连续组的基础上+1
                int nCurrentConsistency = nPreviousConsistency + 1;
                如果上述连续还未记录到 vCurrentConsistentGroups ，那么记录一下
                if (!vbConsistentGroup[iG])
                    将该“子候选组”的该关键帧打上连续编号加入到“当前连续组”
                    ConsistentGroup cg = std::make_pair(spCandidateGroup, nCurrentConsistency)
                    放入本次闭环检测的连续组 vCurrentConsistentGroups 里
                    vCurrentConsistentGroups.push_back(cg)
                    标记一下，防止重复添加到同一个索引iG
                    vbConsistentGroup[iG] = true
                如果连续长度满足要求，那么当前的这个候选关键帧是足够靠谱的
                if (nCurrentConsistency >= mnCovisibilityConsistencyTh && !bEnoughConsistent)
                    记录为达到连续条件了
                    mvpEnoughConsistentCandidates.push_back(pCandidateKF)
                    标记一下，防止重复添加
                    bEnoughConsistent = true
        如果该“子候选组”的所有关键帧和上次闭环无关（不连续），vCurrentConsistentGroups 没有新添加连续关系
        于是就把“子候选组”全部拷贝到 vCurrentConsistentGroups，用于更新mvConsistentGroups，连续性计数器设为0
        if (!bConsistentForSomeGroup)
            ConsistentGroup cg = std::make_pair(spCandidateGroup, 0)
            vCurrentConsistentGroups.push_back(cg)
    更新连续组
    mvConsistentGroups = vCurrentConsistentGroups
```
## 总结一下

```bash
先找到闭环候选帧
循环遍历闭环候选帧
    根据闭环候选帧产生子候选组，也就是与候选帧相连的关键帧组
    循环遍历前一次闭环检测产生的子连续组，称为sPreviousGroup
        循环遍历子候选组
            判断sPreviousGroup中是否存在子候选组的关键帧
        如果存在，则认为该子候选组产生了连续，取出sPreviousGroup的连续性，并在此基础上+1，作为子候选组的连续性
        此外，还需要将这个子候选组作为新的子连续组；
        需要注意：连续组内，所有关键帧不能有相同的情况出现，所以要避免子候选组成为新的子连续组时在同一个sPreviousGroup中

        如果连续长度满足要求，那么当前这个候选关键帧是足够靠谱的，记录下来

更新连续组，将当前帧的子连续组赋值到mvConsistentGroups
```

- 在ORB-SLAM2的闭环检测中，使用Sim3而非SE3进行位姿矫正的核心原因是：‌Sim3能同时处理尺度变化和旋转/平移，而单目SLAM的尺度信息未知‌。闭环检测时，不同帧间的尺度可能不一致（如运动速度变化导致），Sim3通过引入尺度因子s（s=1时为刚体变换）能更准确地统一地图尺度，而SE3无法处理这种尺度变化。

# 三、分析`ComputeSim3`方法

- 目的：计算当前关键帧和闭环候选帧的Sim3变换

- Sim3计算流程：
  - 1. 通过BoW加速描述子的匹配，利用RANSAC粗略地计算出当前帧与闭环帧的Sim3
  - 2. 根据估计的Sim3，对3D点进行投影找到更多匹配，通过优化的方法计算更精确的Sim3
  - 3. 将闭环帧以及闭环帧相连的关键帧的地图点与当前帧的地图点进行匹配

```bash
1. 遍历闭环候选帧，初步筛选出与当前关键帧的匹配特征点大于20的候选帧集合，为每一个候选帧构造一个Sim3Solver
for (KeyFrame* pKF : mvpEnoughConsistentCandidates)
    将当前帧mpCurrentKF与候选帧pKF进行匹配特征点
    int nmatches = matcher.SearchByBoW(mpCurrentKF, pKF, vvpMapPointMatches[i])
    如果匹配的特征点数足够(>20)，为候选帧构造Sim3求解器
    # vvpMapPointMatches[i]是匹配上的特征点
    Sim3Solver* pSolver = new Sim3Solver(mpCurrentKF, pKF, vvpMapPointMatches[i], mbFixScale)
    # Sim3Solver RNASAC过程置信度0.99，至少20个inliers，最多迭代300次迭代
    pSolver->SetRansacParameters(0.99, 20, 300)
    vpSim3Solvers[i] = pSolver
2. 对每一个候选帧用Sim3Solver迭代匹配， 直到有一个候选帧匹配成功，或者全部失败
for (int i = 0; i < nInitialCandidates; i++)
    KeyFrame* pKF = mvpEnoughConsistentCandidates[i]
    Sim3Solver* pSolver = vpSim3Solvers[i]
    Sim3计算匹配，最多迭代5次，返回的Scm是候选帧pKF到当前帧mpCurrentKF的Sim3变换(T12)
    cv::Mat Scm = pSolver->iterate(5, bNoMore, vbInliers, nInliers)
    如果计算出了Sim3变换，继续匹配出更多点并优化。因为之前SearchByBoW匹配可能会有遗漏
        取出经过 Sim3Solver 后匹配点中的内点集合
        std::vector<MapPoint*> vpMapPointMatches(vvpMapPointMatches[i].size(), static_cast<MapPoint*>(NULL));
        for (size_t j = 0, jend = vbInliers.size(); j < jend; j++) {
            if (vbInliers[j]) { // 保存内点
                vpMapPointMatches[j] = vvpMapPointMatches[i][j];
            }
        }
        通过上面求得Sim3变换引导关键帧匹配，弥补Step1中的漏匹配
        候选帧pKF到当前帧 mpCurrentKF 的 R(R12),t(T12)，变换尺度s(s12)
        cv::Mat R = pSolver->GetEstimatedRotation();
        cv::Mat t = pSolver->GetEstimatedTranslation();
        const float s = pSolver->GetEstimatedScale();
        matcher.SearchBySim3(mpCurrentKF, pKF, vpMapPointMatches, s, R, t, 7.5)

        用新的匹配来优化Sim3，只要有一个候选帧通过Sim3的求解与优化，就跳出停止对其它候选帧的判断
        g2o::Sim3 gScm(Converter::toMatrix3d(R), Converter::toVector3d(t), s)
        优化 mpCurrentKF 与 pKF对应的MapPoints间的Sim3，得到优化后的量 gScm
        const int nInliers = Optimizer::OptimizeSim3(mpCurrentKF, pKF, vpMapPointMatches, gScm, 10, mbFixScale)
        如果优化成功，则停止 while 循环遍历闭环候选帧
        if (nInliers >= 20)
            mpMatchedKF 就是最终闭环检测出来与当前帧形成闭环的关键帧
            mpMatchedKF = pKF;
            gSmw:从世界坐标系 w 到该候选帧 m 的Sim3变换，都在一个坐标系下，所以尺度 scale = 1
            g2o::Sim3 gSmw(Converter::toMatrix3d(pKF->GetRotation()), Converter::toVector3d(pKF->GetTranslation()), 1.0);
            得到g2o优化后从世界坐标系到当前帧的Sim3变换
            # gScm：候选帧到当前帧的Sim3变换
            # 所以mg2oScw：就是世界坐标系到当前帧的Sim3变换
            mg2oScw = gScm * gSmw;
            mScw = Converter::toCvMat(mg2oScw);
            mvpCurrentMatchedPoints = vpMapPointMatches;
            break;
3. 取出与当前帧闭环匹配上的关键帧及其共视关键帧，以及这些共视关键帧的地图点
    std::vector<KeyFrame*> vpLoopConnectedKFs = mpMatchedKF->GetVectorCovisibleKeyFrames()
    vpLoopConnectedKFs.push_back(mpMatchedKF)
    for (KeyFrame* pKF : vpLoopConnectedKFs)
        std::vector<MapPoint*> vpMapPoints = pKF->GetMapPointMatches()
        遍历关键帧的所有有效地图点
        for (MapPoint* pMP : vpMapPoints)
            mvpLoopMapPoints.push_back(pMP)
            # 标记一下
            pMP->mnLoopPointForKF = mpCurrentKF->mnId
4. 将闭环关键帧及其连接关键帧的所有地图点投影到当前关键帧进行投影匹配
    根据投影查找更多的匹配（成功的闭环匹配需要满足足够多的匹配特征点数）
    根据Sim3变换，将每个 mvpLoopMapPoints 投影到 mpCurrentKF 上，搜索新的匹配对
    mvpCurrentMatchedPoints 是前面经过 SearchBySim3 得到的已经匹配的点对，这里就忽略不再匹配了
    matcher.SearchByProjection(mpCurrentKF, mScw, mvpLoopMapPoints, mvpCurrentMatchedPoints, 10)
5. 统计当前帧与闭环关键帧的匹配地图点数目，超过40个说明成功闭环，否则失败
    for (size_t i = 0; i < mvpCurrentMatchedPoints.size(); i++)
        if (mvpCurrentMatchedPoints[i])
            nTotalMatches++
    如果当前回环可靠，保留当前待闭环关键帧，其它闭环候选全部删掉以后不用了
    if (nTotalMatches >= 40)
        for (int i = 0; i < nInitialCandidates; i++)
            if (mvpEnoughConsistentCandidates[i] != mpMatchedKF)
                mvpEnoughConsistentCandidates[i]->SetErase()
        return true;
    else    # 闭环不可靠，闭环候选及当前待闭环帧全部删除
        for (int i = 0; i < nInitialCandidates; i++)
            mvpEnoughConsistentCandidates[i]->SetErase()
        return false;
```


gemini或gpt的codex或claude 4.5