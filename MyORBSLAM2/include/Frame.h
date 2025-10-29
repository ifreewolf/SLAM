#pragma once
#ifndef FRAME_H
#define FRAME_H

#include "MapPoint.h"
#include "KeyFrame.h"
#include "ORBVocabulary.h"
#include "ORBextractor.h"

namespace ORB_SLAM2
{

#define FRAME_GRID_ROWS 48
#define FRAME_GRID_COLS 64

class MapPoint;
class KeyFrame;

class Frame
{
public:
    Frame();

    // Copy constructor
    Frame(const Frame &frame);

    // Constructor for stereo cameras.
    Frame(const cv::Mat &imLeft, const cv::Mat &imRight, const double &timeStamp, ORBextractor* extractorLeft, ORBextractor* extractorRight,
          ORBVocabulary* voc, cv::Mat &K, cv::Mat &distCoef, const float &bf, const float &thDepth);

    // Constructor for RGBD cameras.
    Frame(const cv::Mat &imGray, const cv::Mat &imDepth, const double &timeStamp, ORBextractor* extractor,
          ORBVocabulary* voc, cv::Mat &K, cv::Mat &distCoef, const float &bf, const float &thDepth);
    
    // Constructor for Monocular cameras.
    Frame(const cv::Mat &imGray, const double &timeStamp, ORBextractor* extractor,
          ORBVocabulary* voc, cv::Mat &K, cv::Mat &distCoef, const float &bf, const float &thDepth);
    
    // Extract ORB on the image. 0 for left image and 1 for right image.
    void ExtractORB(int flag, const cv::Mat &im);

    // Associate a "right" coordinate to a keypoint if there is valid depth in the depthmap.
    void ComputeStereoFromRGBD(const cv::Mat &imDepth);


public:
    // Vocabulary used for relocalization.
    ORBVocabulary* mpORBvocabulary;
    
    // Feature extractor. The right is used only in the stereo case.
    ORBextractor* mpORBextractorLeft;   // 左目特征点提取器
    ORBextractor* mpORBextractorRight;  // 右目特征点提取器(单目/RGBD模式下为空指针)

    // Frame timestamp
    double mTimeStamp;  // 当前帧的时间戳

    // Calibration matrix and OpenCV distortion parameters.
    cv::Mat mK;         // 相机内参矩阵
    static float fx;
    static float fy;
    static float cx;
    static float cy;
    static float invfx;
    static float invfy;
    cv::Mat mDistCoef;

    // Stereo baseline multiplied by fx.
    float mbf;  // 相机基线与焦距的乘积

    // Stereo baseline in meters.
    float mb;   // 相机基线，双目相机间的距离

    // Threshold close/far points. Close points are inserted from 1 view.
    // Far points are inserted as in the monocular case from 2 views.
    float mThDepth; // 判断单目特征点和双目特征点的阈值；深度低于该值的特征点被认为是双目特征点；深度高于该阈值的特征点被认为是单目特征点 ？？？？？？？？

    // Number of KeyPoints.
    int N;

    // MapPoints associated to keypoints, NULL pointer if no association.
    std::vector<MapPoint*> mvpMapPoints;

    // Vector of keypoints
    // In the stereo case, mvKeysUn is redundant as images must be rectified. Stereo中，图像是经过畸变矫正过的，所以这个变量是冗余的
    // In the RGBD case, RGB images can be distored.
    std::vector<cv::KeyPoint> mvKeys;       // 畸变矫正前的左目特征点
    std::vector<cv::KeyPoint> mvKeysRight;  // 右目特征点
    std::vector<cv::KeyPoint> mvKeysUn;     // 畸变矫正后的左目特征点

    // Corresponding stereo coordinate and depth for each keypoint.
    // "Monocular" keypoints have a negative value.
    std::vector<float> mvuRight;    // 左目特征点在右目中匹配特征点的横坐标（左右目匹配特征点的纵坐标相同）
    std::vector<float> mvDepth;     // 特征点深度
    // 对于单目特征点（单目相机输入的特征点或没有找到右目匹配的左目图像特征点）其mvuRight和mvDepth均为-1
    
    // ORB descriptor, each row associated to a keypoint.
    cv::Mat mDescriptors;       // 左目图像特征点描述子
    cv::Mat mDescriptorsRight;  // 右目图像特征点描述子

    // Flag to identify outlier associations.
    std::vector<bool> mvbOutlier;

    // Keypoints are assigned to cells in a grid to reduce matching complexity when projecting MapPoints.
    // 每个格子分配的特征点数，将图像分为格子，保证提取的特征点比较均匀
    // FRAME_GRID_ROWS 48
    // FRAME_GRID_COLS 64
    // 这个向量中存储的是每个图像网格内特征点的id (左图)
    static float mfGridElementWidthInv;
    static float mfGridElementHeightInv;
    std::vector<std::size_t> mGrid[FRAME_GRID_COLS][FRAME_GRID_ROWS];
    
    // Camera pose.
    cv::Mat mTcw;
    
    // Current and Next Frame id.
    static long unsigned int nNextId;
    long unsigned int mnId;

    // Scale pyramid info.
    int                mnScaleLevels;       // 
    float              mfScaleFactor;
    float              mfLogScaleFactor;
    std::vector<float> mvScaleFactors;
    std::vector<float> mvInvScaleFactors;
    std::vector<float> mvLevelSigma2;
    std::vector<float> mvInvLevelSigma2;

    // Undistorted Image Bounds (computed once).
    static float mnMinX;
    static float mnMaxX;
    static float mnMinY;
    static float mnMaxY;

    static bool mbInitialComputations;  // 是否需要初始化Frame相机参数，程序初始化时为true，第一次为相机参数赋值后变为false

private:
    // Undistort keypoints given OpenCV distortion parameters.
    // Only for the RGBD case. Stereo must be already rectified! (called in the constructor).
    void UndistortKeyPoints();

    // Computes image bounds for the undistorted image (called in the constructor).
    void ComputeImageBounds(const cv::Mat &imLeft);

    // Assign keypoints to the grid for speed up feature matching (called in the constructor).
    void AssignFeaturesToGrid();

    // Rotation, translation and camera center
    cv::Mat mRcw;
    cv::Mat mtcw;
    cv::Mat mRwc;
    cv::Mat mOw;    // == mtwc
};

}

#endif