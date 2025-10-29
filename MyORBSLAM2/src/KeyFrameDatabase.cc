#include "KeyFrameDatabase.h"

namespace ORB_SLAM2
{

KeyFrameDatabase::KeyFrameDatabase(const ORBVocabulary &voc): mpVoc(&voc)
{
    mvInvertedFile.resize(voc.size());
}

void KeyFrameDatabase::add(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lock(mMutex);

    for (DBoW2::BowVector::const_iterator vit = pKF->mBowVec.begin(), vend = pKF->mBowVec.end(); vit != vend; vit++) {
        mvInvertedFile[vit->first].emplace_back(pKF);
    }
}


/**
 * mBowVec应该是一个
 */
void KeyFrameDatabase::erase(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lock(mMutex);

    // Erase elements in the Inverse File for the entry
    for (DBoW2::BowVector::const_iterator vit = pKF->mBowVec.begin(), vend = pKF->mBowVec.end(); vit != vend; vit++) {
        // List of keyframes that share the word
        std::list<KeyFrame*> &lKFs = mvInvertedFile[vit->first];

        for (std::list<KeyFrame*>::iterator lit = lKFs.begin(), lend = lKFs.end(); lit != lend; lit++) {
            if (pKF == *lit) {
                lKFs.erase(lit);
                break;
            }
        }
    }
}


void KeyFrameDatabase::clear()
{
    mvInvertedFile.clear();
    mvInvertedFile.resize(mpVoc->size());
}


/**
 * @brief 寻找当前关键帧的闭环候选关键帧
 * 
 */
std::vector<KeyFrame*> KeyFrameDatabase::DetectLoopCandidates(KeyFrame* pKF, float minScore)
{
    // Step1 找出当前关键帧的所有共视关键帧
    std::set<KeyFrame*> spConnectedKeyFrames = pKF->GetConnectedKeyFrames();

    // Search all keyframes that share a word with current keyframes
    // Discard keyframes connected to the query keyframe
    // Step2 找出所有具有相同BoW但不直接相连的关键帧
    std::list<KeyFrame*> lKFsSharingWords;  // 存储闭环候选关键帧
    {
        std::unique_lock<std::mutex> lock(mMutex);
        // 遍历当前关键帧的所有BoW词向量
        for (DBoW2::BowVector::const_iterator vit = pKF->mBowVec.begin(), vend = pKF->mBowVec.end(); vit != vend; vit++) {
            std::list<KeyFrame*> &lKFs = mvInvertedFile[vit->first];    // 根据BoW Tree叶子节点的Id获取所有具有相同叶子节点的帧

            for (std::list<KeyFrame*>::iterator lit = lKFs.begin(), lend = lKFs.end(); lit != lend; lit++) {
                KeyFrame* pKFi = *lit;
                if (pKFi->mnLoopQuery != pKF->mnId) {   // 首次被pKF查找候选帧过程中被访问
                    pKFi->mnLoopWords = 0;
                    if (!spConnectedKeyFrames.count(pKFi)) {    // pKFi不在PKF的共视关键帧内
                        pKFi->mnLoopQuery = pKF->mnId;
                        lKFsSharingWords.push_back(pKFi);
                    }
                }
                pKFi->mnLoopWords++;
            }
        }
    }

    if (lKFsSharingWords.empty()) {
        return std::vector<KeyFrame*>();
    }

    std::list<std::pair<float, KeyFrame*>> lScoreAndMatch;

    // Only compare against those keyframes that share enough words
    int maxCommonWords = 0;
    for (std::list<KeyFrame*>::iterator lit = lKFsSharingWords.begin(), lend = lKFsSharingWords.end(); lit != lend; lit++) {
        if ((*lit)->mnLoopWords > maxCommonWords) {
            maxCommonWords = (*lit)->mnLoopWords;
        }
    }

    int minCommonWords = maxCommonWords * 0.8f;

    int nscores = 0;

    for (std::list<KeyFrame*>::iterator lit = lKFsSharingWords.begin(), lend = lKFsSharingWords.end(); lit != lend; lit++) {
        KeyFrame* pKFi = *lit;
        if (pKFi->mnLoopWords > minCommonWords) {
            nscores++;
            float si = mpVoc->score(pKF->mBowVec, pKFi->mBowVec);

            pKFi->mLoopScore = si;
            if (si >= minScore) {
                lScoreAndMatch.push_back(std::make_pair(si, pKFi));
            }
        }
    }

    if (lScoreAndMatch.empty()) {
        return std::vector<KeyFrame*>();
    }

    std::list<std::pair<float, KeyFrame*>> lAccScoreAndMatch;
    float bestAccScore = minScore;

    for (std::list<std::pair<float, KeyFrame*>>::iterator lit = lScoreAndMatch.begin(), lend = lScoreAndMatch.end(); lit != lend; lit++) {
        KeyFrame* pKFi = lit->second;
        std::vector<KeyFrame*> vpNeighs = pKFi->GetBestCovisibilityKeyFrames(10);

        float bestScore = lit->first;
        float accScore = lit->first;
        KeyFrame* pBestKF = pKFi;
        for (std::vector<KeyFrame*>::iterator vit = vpNeighs.begin(), vend = vpNeighs.end(); vit != vend; vit++) {
            KeyFrame* pKF2 = *vit;
            if (pKF2->mnLoopQuery == pKF->mnId && pKF2->mnLoopWords > minCommonWords) {
                accScore += pKF2->mLoopScore;
                if (pKF2->mLoopScore > bestScore) {
                    pBestKF = pKF2;
                    bestScore = pKF2->mLoopScore;
                }
            }
        }

        lAccScoreAndMatch.push_back(std::make_pair(accScore, pBestKF));
        if (accScore > bestAccScore) {
            bestAccScore = accScore;
        }
    }

    float minScoreToRetain = 0.75f * bestAccScore;

    std::set<KeyFrame*> spAlreadyAddedKF;
    std::vector<KeyFrame*> vpLoopCandidates;
    vpLoopCandidates.reserve(lAccScoreAndMatch.size());

    for (std::list<std::pair<float, KeyFrame*>>::iterator lit = lAccScoreAndMatch.begin(), lend = lAccScoreAndMatch.end(); lit != lend; lit++) {
        if (lit->first > minScoreToRetain) {
            KeyFrame* pKFi = lit->second;
            if (!spAlreadyAddedKF.count(pKFi)) {
                vpLoopCandidates.push_back(pKFi);
                spAlreadyAddedKF.insert(pKFi);
            }
        }
    }

    return vpLoopCandidates;
}


/**
 * @brief 在重定位中找到与该帧相似的候选关键帧
 * Step1 找出和当前帧具有公共单词的所有关键帧
 * Step2 只和具有共同单词较多的关键帧进行相似度计算
 * Step3 将与关键帧相连（权值最高）的前十个关键帧归为一组，计算累计得分
 * Step4 只返回累计得分较高的组中分数最高的关键帧
 * 
 * @param F 需要重新定位的帧
 * @return 相似的候选关键帧数组
 */
std::vector<KeyFrame*> KeyFrameDatabase::DetectRelocalizationCandidates(Frame* F)
{
    std::list<KeyFrame*> lKFsSharingWords;

    // Search all keyframes that share a word with current frame
    // Step1 找出和当前帧具有公共单词(word)的所有关键帧
    {
        std::unique_lock<std::mutex> lock(mMutex);

        // mBowVec 内部实际存储的是 std::map<WordId, WordValue>
        // WordId 和 WordValue 表示word在叶子中的 id 和权重
        // 循环遍历需要重定位帧F所有特征点的词袋向量
        for (DBoW2::BowVector::const_iterator vit = F->mBowVec.begin(), vend = F->mBowVec.end(); vit != vend; vit++) {
            // 根据倒排索引，提取所有包含该wordid的所有KeyFrame
            std::list<KeyFrame*> &lKFs = mvInvertedFile[vit->first];

            for (std::list<KeyFrame*>::iterator lit = lKFs.begin(), lend = lKFs.end(); lit != lend; lit++) {
                KeyFrame* pKFi = *lit;
                // pKFi->mnRelocQuery 起标记作用，是为了防止重复选取，表示被重新定位使用的关键帧序号
                if (pKFi->mnRelocQuery != F->mnId) {
                    // pKFi还没有被标记为F的重定位候选帧
                    pKFi->mnRelocWords = 0;
                    pKFi->mnRelocQuery = F->mnId;
                    lKFsSharingWords.push_back(pKFi);
                }
                pKFi->mnRelocWords++;   // mnRelocWords记录被访问的次数，是不是意味着，同一个关键帧可以在不同的node下面。因为词袋树是按照描述子来划分的
                                        // 准确来说，是在一个重定位帧下，被不同特征点重复访问的次数; 同时在不同的node下面也是正常的，因为一帧图像有多个特征点，但特征点会属于不同的node
                                        // 作用：用来统计当前帧F与候选帧pKFi之间的共同单词数，可以体现他们的相似度
            }
        }
    }
    // 如果和当前帧具有公共单词的关键帧数目为0，无法进行重定位，返回空
    if (lKFsSharingWords.empty()) {
        return std::vector<KeyFrame*>();
    }

    // Step2 统计上述关键帧中与当前帧F具有共同单词最多的单词数 maxCommonWords，用来设定阈值1
    int maxCommonWords = 0;
    for (std::list<KeyFrame*>::iterator lit = lKFsSharingWords.begin(), lend = lKFsSharingWords.end(); lit != lend; lit++) {
        if ((*lit)->mnRelocWords > maxCommonWords) {
            maxCommonWords = (*lit)->mnRelocWords;
        }
    }

    // 阈值1：最小公共单词数位最大公共单词数目的0.8倍
    int minCommonWords = maxCommonWords * 0.8f;

    std::list<std::pair<float, KeyFrame*>> lScoreAndMatch;

    int nscores = 0;

    // Step3 遍历上述关键帧，挑选出共有单词数大于阈值1及其和当前帧单词得分存入lScoreAndMatch中
    for (std::list<KeyFrame*>::iterator lit = lKFsSharingWords.begin(), lend = lKFsSharingWords.end(); lit != lend; lit++) {
        KeyFrame* pKFi = *lit;

        // 当前帧F只和具有公共单词较多的关键帧进行比较
        if (pKFi->mnRelocWords > minCommonWords) {
            nscores++;
            // 用mBowVec来计算两者的相似度得分
            float si = mpVoc->score(F->mBowVec, pKFi->mBowVec);
            pKFi->mRelocScore = si;
            lScoreAndMatch.push_back(std::make_pair(si, pKFi));
        }
    }

    if (lScoreAndMatch.empty()) {
        return std::vector<KeyFrame*>();
    }

    std::list<std::pair<float, KeyFrame*>> lAccScoreAndMatch;
    float bestAccScoree = 0;

    // Step4 计算 lScoreAndMatch 中每个关键帧的共视关键帧组的总得分，得到最高组得分 bestAccScore，并依次决定阈值2
    for (std::list<std::pair<float, KeyFrame*>>::iterator lit = lScoreAndMatch.begin(), lend = lScoreAndMatch.end(); lit != lend; lit++) {
        KeyFrame* pKFi = lit->second;
        // 取出与关键帧pKFi共视程度最高的前10个关键帧
        std::vector<KeyFrame*> vpNeighs = pKFi->GetBestCovisibilityKeyFrames(10);

        // 该组最高得分
        float bestScore = lit->first;
        // 该组累计得分
        float accScore = bestScore;
        // 该组最高分数对应的关键帧
        KeyFrame* pBestKF = pKFi;

        // 遍历共视关键帧，累计得分
        for (std::vector<KeyFrame*>::iterator vit = vpNeighs.begin(), vend = vpNeighs.end(); vit != vend; vit++) {
            KeyFrame* pKF2 = *vit;
            if (pKF2->mnRelocQuery != F->mnId) {    // 非候选帧，则退出，因为只有pKF2也在重定位候选帧中，才能贡献分数
                continue;
            }

            accScore += pKF2->mRelocScore;

            // 统计得到组里分数最高的KeyFrame
            if (pKF2->mRelocScore > bestScore) {
                pBestKF = pKF2;
                bestScore = pKF2->mRelocScore;
            }
        }
        lAccScoreAndMatch.push_back(std::make_pair(accScore, pBestKF));

        // 记录所有组中最高的得分
        if (accScore > bestAccScoree) {
            bestAccScoree = accScore;
        }
    }

    // Step5 得到所有组中总得分大于阈值2的，组内得分最高的关键帧，作为候选关键帧组
    // 阈值2：最高得分的0.75倍
    float minScoreToRetain = 0.75f * bestAccScoree;
    std::set<KeyFrame*> spAlreadyAddedKF;
    std::vector<KeyFrame*> vpRelocCandidates;
    vpRelocCandidates.reserve(lAccScoreAndMatch.size());
    for (std::list<std::pair<float, KeyFrame*>>::iterator lit = lAccScoreAndMatch.begin(), lend = lAccScoreAndMatch.end(); lit != lend; lit++) {
        const float &si = lit->first;
        // 只返回累计得分大于阈值2的组中分数最高的关键帧
        if (si > minScoreToRetain) {
            KeyFrame* pKFi = lit->second;
            // 判断pKFi是否已经存在队列中
            if (!spAlreadyAddedKF.count(pKFi)) {
                vpRelocCandidates.push_back(pKFi);
                spAlreadyAddedKF.insert(pKFi);
            }
        }
    }

    return vpRelocCandidates;
}

}