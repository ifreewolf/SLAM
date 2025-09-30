#pragma once
#ifndef ORBEXTRACTOR_H
#define ORBEXTRACTOR_H

#include "common.h"

namespace ORB_SLAM2
{
class ExtractorNode
{
public:
    ExtractorNode(): bNoMore(false) {}

    void DivideNode(ExtractorNode &n1, ExtractorNode &n2, ExtractorNode &n3, ExtractorNode &n4);

    std::vector<cv::KeyPoint>  vKeys;
    cv::Point2i UL;
    cv::Point2i UR;
    cv::Point2i BL;
    cv::Point2i BR;
    std::list<ExtractorNode>::iterator lit;
    bool bNoMore;
};

class ORBextractor
{
public:
    enum { HARRIS_SCORE=0, FAST_SCORE=1 };

    ORBextractor(int nfeatures, float scaleFactor, int nlevels, int iniThAST, int minThFAST);

    ~ORBextractor() {}

    // Compute the ORB features and descriptors on an image.
    // ORB are dispersed on the image using an octree. ORB特征点通过八叉树结构在图像上均匀分布, dispersed 分布
    // Mask is ignored in the current implementation.
    void operator()(cv::InputArray image, cv::InputArray mask, std::vector<cv::KeyPoint>& keypoints, cv::OutputArray descriptors);

    int inline GetLevels() {
        return mnLevels;
    }

    float inline GetScaleFactor() {
        return mdScaleFactors;
    }

    std::vector<float> inline GetScaleFactors() {
        return mvScaleFactor;
    }

    std::vector<float> inline GetInverseScaleFactors() {
        return mvInvScaleFactor;
    }

    std::vector<float> inline GetScaleSigmaSquares() {
        return mvLevelSigma2;
    }

    std::vector<float> inline GetInverseScaleSigmaSquares() {
        return mvInvLevelSigma2;
    }

    std::vector<cv::Mat> mvImagePyramid;

protected:
    void ComputePyramid(cv::Mat image);
    void ComputeKeyPointsOctTree(std::vector<std::vector<cv::KeyPoint>>& allKeypoints);
    std::vector<cv::KeyPoint> DistributeOctTree(const std::vector<cv::KeyPoint>& vToDistributeKeys,
                                                const int &minX, const int &maxX, const int &minY, const int &maxY, const int &N, const int &level);

    std::vector<cv::Point> mvPattern; // 生成特征点的描述子的采样坐标

    int    mnFeatures;      // 所有层级提取到的特征点数之和 1000
    double mdScaleFactors;  // 图像金字塔相邻层级间的缩放系数 1.2
    int    mnLevels;        // 金字塔层级数 8
    int    mnIniThFAST;     // 提取特征点描述子门槛(高) 20
    int    mnMinThFAST;     // 提取特征点描述子门槛(低) 7

    std::vector<int>   mnFeaturesPerLevel;  // 金字塔每层提取的特征点数，{61, 73, 87, 105, 126, 151, 181, 216}

    std::vector<int>   mvUmax;              // uv图像坐标系，u表示横轴，该变量是横轴能取的最大值

    std::vector<float> mvScaleFactor;       // 各层级点的缩放系数       {1, 1.2, 1.44, 1.728, 2.074, 2.488, 2.986, 3.583}
    std::vector<float> mvInvScaleFactor;    // 各层级缩放系数的倒数     {1, 0.833, 0.694, 0.579, 0.482, 0.402, 0.335, 0.2791}
    std::vector<float> mvLevelSigma2;       // 各层级缩放系数的平方     {1, 1.44, 2.074, 2.986, 4.300, 6.190, 8.916, 12.838}
    std::vector<float> mvInvLevelSigma2;    // 各层级缩放系数平方的倒数 {1, 0.694, 0.482, 0.335, 0.233, 0.162, 0.112, 0.078}

};
}

#endif