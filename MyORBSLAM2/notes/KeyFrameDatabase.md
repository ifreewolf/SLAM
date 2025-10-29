该类存在两个比较重要的成员函数，分别是`DetectLoopCandidates`和`DetectRelocalizationCandidates`，一个是寻找当前关键帧的闭环候选关键帧，另一个是寻找当前帧的重定位候选帧，它们的流程有众多相似之处，下面详细分析其流程：

## 1. `DetectLoopCandidates`

```bash
1. 找出当前关键帧的所有共视关键帧（寻找的候选帧不能是当前关键帧的共视关键帧）
    std::set<KeyFrame*> spConnectedKeyFrames = pKF->GetConnectedKeyFrames()
2. 根据当前关键帧的BoW，寻找所有具有相同BoW的关键帧
    for (DBoW2::BowVector : pKF->mBowVec) # 遍历pKF所有特征点的mBowVec
        std::list<KeyFrame*> &lKFs = mnInvertedFile[vit->first];    # 所有具有相同BoW的关键帧
        for (KeyFrame pKFi : lKFs) # 遍历具有相同BoW的关键帧
            pKFi->mnLoopQuery != pKF->mnId # 未曾被pKF查询过(用于搜索闭环候选关键帧)
                pKFi->mnLoopWords = 0   # pKFi被当前帧pKF查询过的次数，因为不同特征点BoW可能会存在于同一个帧
                if (!spConnectedKeyFrames.count(pKFi))  # 该候选帧不是pKF的共视关键帧
                    pKFi->mnLoopQuery = pKF->mnId # 标记在pKF查询过程中该pKFi被标记了
                    lKFsSharingWords.push_back(pKFi) # 存储该候选帧
            pKFi->mnLoopWords++ # 计数，查询在pKF查找候选帧过程中pKFi总共被访问了多少次，可以反映与pKF的相似度
3. 获取所有候选关键帧中被访问最多的次数，也就是最大的mnLoopWords
    for (KeyFrame kf : lKFsSharingWprds)
        maxCommonWords = max(maxCommonWords, kf->mnLoopWords)
4. 根据步骤3获取的maxCommonWords从候选帧中进一步筛查，并计算候选帧和pKFi的相似度分数
    int minCommonWords = maxCommonWords * 0.8f; # 根据maxCommonWords获得阈值1
    for (KeyFrame pKFi : lKFsSharingWords)        # 循环遍历候选帧
        if (pKFi->mnLoopWords > minCommonWords)   # 候选帧被访问次数大于阈值1
            float si = mpVoc->score(pKF->mBowVec, pKFi->mBowVec)    # 计算候选帧和pKF的相似度得分
            pKFi->mLoopScore = si   # 将相似度得分保存在候选帧的mLoopScore变量中
            if (si >= minScore) # minScore是调用该函数外部传入的阈值
                lScoreAndMatch.push_back(std::make_pair(si, pKFi)); # 新的候选帧列表，包含分数和候选帧
5. 对每个候选帧搜索其共视程度最高的前10个共视帧，然后从中选择与pKF相似度最高的关键帧作为新的候选关键帧
    for (std::pair<float, KeyFrame*> pfk : lScoreAndMatch)
        pKFi = pfk->second; # 候选帧
        std::vector<KeyFrame*> vpNeighs = pKFi->GetBestCovisibilityKeyFrames(10) # 候选帧的共视关键帧前10
        for (KeyFrame pKF2 : vpNeights) # 循环遍历候选帧的共视关键帧
            if (pKF2->mnLoopQuery == pKF->mnId && pKF2->mnLoopWords > minCommonWords)   # 满足候选帧是当前帧pKF搜索过的,同时候选帧被访问的次数要大于阈值1
                accScore += pKF2->mLoopScore    # 累计候选帧的相似度得分
                if (pKF2->mLoopScore > bestScore) # 这里是为了获取候选帧pKFi共视关键帧与pKF相似度得分
                    pBestKF = pKF2                  # 记录pKFi候选帧组中与pKF相似度得分最高一帧
                    bestScore = pKF2->mLoopScore    # 对应的相似度得分
        lAccScoreAndMatch.push_back(make_pair(accScore, pBestKF))   # 重新记录，这里记录的得分是整个候选帧共视帧组的得分
                                                                    # 这里记录的帧理论上都是lScoreAndMatch上的，因为要求被pKF访问过，pKF2->mnLoopQuery == pKF->mnId
                                                                    # pKF2->mnLoopWords > minCommonWords 这个条件好像没有必要，因为首先是要求pKF2->mnLoopQuery == pKF->mnId；
                                                                    # 但lScoreAndMatch中的候选帧是已经筛选过大于minCommonWords的，除非有一种可能：被pKF访问过，但没有被lScoreAndMatch选中过
        if (accScore > bestAccScore)
            bestAccScore = accScore # 记录候选帧组的最高得分
6. 对候选帧再次筛选，选择满足阈值2的候选帧
    float minScoreToRetain = 0.75f * bestAccScore   # 创建阈值2
    for (std::pair<float, KeyFrame*> pfk : lAccScoreAndMatch)
        if (pfk->first > minScoreToRetain)
            pKFi = pfk->second  # 候选帧
            vpLoopCandidates.push_back(pKFi) # 存储最终的候选帧
return vpLoopCandidates;
```

## 2. `DetectRelocalizationCandidates`

- 寻找重定位候选帧

```bash
1. 根据当前帧特征点的BoW，寻找所有具有相同BoW的关键帧
    for (DBoW2::BowVector vit: F->mBowVec)
        lKFs = mvInvertedFile[vit->first] # 获得所有具有相同BoW的特征点
        for (KeyFrame* pKFi : lKFs) # 循环遍历所有具有相同BoW的关键帧
            if (pKFi->mnRelocQuery != F->mnId) # 如何未被帧F访问过
                pKFi->mnRelocWords = 0          # 被F重定位访问次数置为0，因为是首次访问
                pKFi->mnRelocQuery = F->mnId    # 记录被帧F访问
                lKFsSharingWords.push_back(pKFi) # 在第一次访问时保存到lKFsSharingWords作为候选帧
            pKFi->mnRelocWords++    # 记录被F访问次数累积
2. 查询所有候选关键帧中，被访问次数最多的候选帧的次数
    for (KeyFrame* pKFi : lKFsSharingWords)
        maxCommonWords = max(maxCommonWords, pKFi->mnRelocWords)
3. 通过maxCommonWords构建阈值1，所有访问次数大于阈值1的候选帧被重新选中，并计算它与帧F的相似度
    int minCommonWords = maxCommonWords * 0.8f # 构造阈值1
    for (KeyFrame* pKFi : lKFsSharingWords) # 循环遍历候选帧
        if (pKFi->mnRelocWords > minCommonWords) # 根据访问次数来进一步选择候选帧
            float si = mpVoc->score(F->mBowVec, pKFi->mBowVec)  # 计算候选帧与帧F的相似度得分
            pKFi->mRelocScore = si; # 将得分赋值给候选帧的mRelocScore成员变量
            lScoreAndMatch.push_back(std::make_pair(si, PKFi))  # 将候选帧构造成一个pair，并保存
4. 候选帧的前10共视关键帧作为一个组，计算这个组的总得分，并查找这个组中与帧F相似度最高得分的帧
    for (std::pair<float, KeyFrame*> lit : lScoreAndMatch)
        pKFi = lit->second  # 候选帧
        vpNeighs = pKFi->GetBestCovisibilityKeyFrames(10)   # 候选帧的共视关键帧
        for (KeyFrame* pKF2 : vpNeighs) # 循环遍历候选帧的共视关键帧
            if (pKF2->mnRelocQuery == F->mnId)  # 表示被帧F访问过，为什么这里不要求访问次数大于 minCommonWords？？？？,pKF2有可能被帧F访问过，但没有被lScoreAndMatch选中过
                accScore += pKF2->mRelocScore   # 累积共视关键帧的得分
                if (pKF2->mRelocScore > bestScore) # 统计这个组中最高相似度得分
                    pBestKF = pKF2 # 记录最高得分的帧
                    bestScore = pKF2->mRelocScore   # 记录最高得分
        lAccScoreAndMatch.push_back(std::make_pair(accScore, pBestKF)) # 新的候选帧列表
        if (accScore > bestAccScoree)
            bestAccScoree = accScore    # 记录最高得分的组
5. 根据候选帧组的最高得分构建阈值2，再次进行筛选
    float minScoreToRetain = 0.75f * bestAccScore
    for (std::pair<float, KeyFrame*> pkf : lAccScoreAndMatch)
        if (pkf->first > minScoreToRetain)
            KeyFrame* pKFi = pkf->second
            vpLoopCandidates.push_back(pKFi)
6. return vpLoopCandidates;
```