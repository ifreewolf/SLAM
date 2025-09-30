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

}