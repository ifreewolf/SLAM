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

    // Matching to triangulate new MapPoints. Check Epipolar Constraint.
    int SearchForTriangulation(KeyFrame* pKF1, KeyFrame* pKF2, cv::Mat F12, std::vector<std::pair<size_t, size_t>> &vMatchedPairs, const bool bOnlyStereo);

    // Search maches between MapPoints seen in KF1 and KF2 transforming by a Sim3 [s12*R12|t12]
    // In the stereo and RGBD case, s12 = 1
    int SearchBySim3(KeyFrame* pKF1, KeyFrame* pKF2, std::vector<MapPoint*> &vpMatches12, const float &s12, const cv::Mat &R12, const cv::Mat &t12, const float th);

    // Project MapPoint into KeyFrame and search for duplicated MapPoints.
    int Fuse(KeyFrame* pKF, const std::vector<MapPoint*> &vpMapPoints, const float th = 3.0);

    // Project MapPoints into KeyFrame using a given Sim3 and Search for duplicated MapPoints.
    int Fuse(KeyFrame* pKF, cv::Mat Scw, const std::vector<MapPoint*> &vpPoints, float th, std::vector<MapPoint*> &vpReplacePoint);

public:
    static const int TH_LOW;
    static const int TH_HIGH;
    static const int HISTO_LENGTH;

protected:

    bool CheckDistEpipolarLine(const cv::KeyPoint &kp1, const cv::KeyPoint &kp2, const cv::Mat &F12, const KeyFrame *pKF);

    void ComputeThreeMaxima(std::vector<int>* histo, const int L, int &ind1, int &ind2, int &ind3);

    float RadiusByViewingCos(const float &viewCos);

    float mfNNratio;
    bool mbCheckOrientation;
};
}

#endif