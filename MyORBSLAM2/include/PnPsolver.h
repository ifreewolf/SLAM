#pragma once
#ifndef PNPSOLVER_H
#define PNPSOLVER_H

#include "common.h"
#include "Frame.h"
#include "MapPoint.h"

namespace ORB_SLAM2
{

class PnPsolver
{
public:
    PnPsolver(const Frame &F, const std::vector<MapPoint*> &vpMapPointMatches);

    ~PnPsolver();

    void SetRansacParameters(double probability = 0.99, // 用于计算RANSAC迭代次数理论值的概率
                             int minInliers = 8,        // 
                             int maxIterations = 300,   //
                             int minSet = 4,            //
                             float epsilon = 0.4,       //
                             float th2 = 5.991);        // 

    cv::Mat find(std::vector<bool> &vbInliers, int &nInliers);

    cv::Mat iterate(int nIterations, bool &bNoMore, std::vector<bool> &vbInliers, int &nInliers);

private:
    void CheckInliers();
    bool Refine();
    
    // Functions from the original EPnP code
    void set_maximum_number_of_correspondences(const int n);
    void reset_correspondences(void);
    void add_correspondence(const double X, const double Y, const double Z, const double u, const double v);
    double compute_pose(double R[3][3], double T[3]);

private:
    double uc;  // 像素偏移，横向，内参
    double vc;  // 像素偏移，纵向，内参
    double fu;  // 横向缩放*焦距，内参
    double fv;  // 纵向缩放*焦距，内参

    double *pws;    // 3D点在世界坐标系下的坐标
    double *us;     // 图像坐标系下的2D点坐标
    double *alphas; // 真实3D点用4个虚拟控制点表示时的系数
    double *pcs;    // 3D点在camera坐标系下的坐标

    int maximum_number_of_correspondences;
    int number_of_correspondences;

    std::vector<MapPoint*> mvpMapPointMatches;

    // 2D Points
    std::vector<cv::Point2f> mvP2D;
    std::vector<float> mvSigma2;

    // 3D Points
    std::vector<cv::Point3f> mvP3Dw;

    // Index inFrame
    std::vector<size_t> mvKeyPointIndices;

    // Current Estimation
    double mRi[3][3];
    double mti[3];
    cv::Mat mTcwi;
    std::vector<bool> mvbInliersi;
    int mnInliersi;

    // Current Ransac State
    int mnIterations;
    std::vector<bool> mvbBestInliers;
    int mnBestInliers;
    cv::Mat mBestTcw;

    // Refined
    cv::Mat mRefinedTcw;
    std::vector<bool> mvbRefinedInliers;
    int mnRefinedInliers;

    // Number of Correspondences
    int N;

    // Indices for random selection [0 ... N-1]
    std::vector<size_t> mvAllIndices;

    // RANSAC probability
    double mRansacProb;

    // RANSAC min inliers
    int mRansacMinInliers;

    // RANSAC max iterations
    int mRansacMaxIts;

    // RANSAC expected inliers/total ratio
    float mRansacEpsilon;

    // RANSAC Minimum Set used at each iteration
    int mRansacMinSet;

    // Max square error associated with scale level. Max error = th*th*sigma(level)*sigma(level)
    std::vector<float> mvMaxError;
};
    
} // namespace ORB_SLAM2


#endif