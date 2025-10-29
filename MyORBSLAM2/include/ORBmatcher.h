#pragma once
#ifndef ORBMATCHER_H
#define ORBMATCHER_H

#include "common.h"

#include "MapPoint.h"
#include "KeyFrame.h"
#include "Frame.h"


namespace ORB_SLAM2
{

class Frame;

class ORBmatcher
{
public:
    ORBmatcher(float nnratio = 0.6, bool checkOri = true);

    // Computes the Hamming distance between two ORB descriptors
    static int DescriptorDistance(const cv::Mat &a, const cv::Mat &b);

    int SearchByProjection(Frame &F, const std::vector<MapPoint*> &vpMapPoints, const float th = 3);

    int SearchByProjection(Frame &currentFrame, const Frame &LastFrame, const float th, const bool bMono);

    int SearchByProjection(Frame &CurrentFrame, KeyFrame* pKF, const std::set<MapPoint*> &sAlreadyFound, const float th, const int ORBdist);

    int SearchByProjection(KeyFrame* pKF, cv::Mat Scw, const std::vector<MapPoint*> &vpPoints, std::vector<MapPoint*> &vpMatched, int th);

    // 
    int SearchByBoW(KeyFrame *pKF, Frame &F, std::vector<MapPoint*> &vpMapPointMatches);
    int SearchByBoW(KeyFrame *pKF1, KeyFrame *pKF2, std::vector<MapPoint*> &vpMatches12);

public:
    static const int TH_LOW;
    static const int TH_HIGH;
    static const int HISTO_LENGTH;

protected:

    void ComputeThreeMaxima(std::vector<int>* histo, const int L, int &ind1, int &ind2, int &ind3);

    float RadiusByViewingCos(const float &viewCos);

    float mfNNratio;
    bool mbCheckOrientation;
};
}

#endif