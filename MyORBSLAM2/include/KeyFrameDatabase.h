#pragma once
#ifndef KEYFRAMEDATABASE_H
#define KEYFRAMEDATABASE_H

#include "common.h"
#include "KeyFrame.h"
#include "Frame.h"
#include "ORBVocabulary.h"

namespace ORB_SLAM2
{

class KeyFrame;
class Frame;

class KeyFrameDatabase
{
public:
    KeyFrameDatabase(const ORBVocabulary &voc);

    void add(KeyFrame* pKF);

    void erase(KeyFrame* pKF);

    void clear();

    // Loop Detection
    // std::vector<KeyFrame *> DetectLoopCandidates(KeyFrame* pKF, float minScore);

    // Relocalization
    // std::vector<KeyFrame *> DetectRelocalizationCandidates(Frame* F);

protected:
    // Associated vocabulary
    const ORBVocabulary* mpVoc; // 相关的字典

    // Inverted file
    std::vector<std::list<KeyFrame*>> mvInvertedFile; // 反向的文件

    // Mutex
    std::mutex mMutex;
};

} // namespace ORB_SLAM

#endif