#pragma once
#ifndef KEYFRAME_H
#define KEYFRAME_H

#include "common.h"
#include "3rdparty/DBoW2/DBoW2/BowVector.h"
#include "3rdparty/DBoW2/DBoW2/FeatureVector.h"

namespace ORB_SLAM2
{

class KeyFrame
{
public:
    // KeyFrame(Frame &F, Map* pMap, KeyFrameDatabase* pKFDB);

    // // Pose functions
    // void SetPose(const cv::Mat &Tcw);
    // cv::Mat GetPose();
    // cv::Mat GetPoseInverse();
    // cv::Mat GetCameraCenter();
    // cv::Mat GetStereoCenter();
    // cv::Mat GetRotaion();
    // cv::Mat GetTranslation();

    // // Bag of Words Representation
    // void ComputeBow();

    // // Covisibility graph function, 共试图函数
    // void AddConnection(KeyFrame* pKF, const int &weight); // 添加共视关键帧
    // void EraseConnection(KeyFrame* pKF); // 删除共视关键帧
    // void UpdateConnections(); // 基于当前关键帧对地图点的观测构造共视图
    // void UpdateBestCovisibles(); // 基于共视图信息修改对应变量
    // std::set<KeyFrame *> GetConnectedKeyFrames();
    // std::vector<KeyFrame *> GetVectorCovisibleKeyFrames();
    // std::vector<KeyFrame *> GetBestCovisiblityKeyFrames(const int &N);
    // std::vector<KeyFrame *> GetCovisiblesByWeight(const int &w);
    // int GetWeight(KeyFrame* pKF);

public:

    static long unsigned int nNextId;

    // BoW
    // Bag-of-Words Vector，词袋向量
    // 用于加速特征匹配和场景识别
    DBoW2::BowVector mBowVec;
};

}

#endif