#pragma once
#ifndef KEYFRAME_H
#define KEYFRAME_H

#include "common.h"
#include "3rdparty/DBoW2/DBoW2/BowVector.h"
#include "3rdparty/DBoW2/DBoW2/FeatureVector.h"

#include "Frame.h"
#include "Map.h"
#include "KeyFrameDatabase.h"

namespace ORB_SLAM2
{

class Frame;
class MapPoint;
class Map;
class KeyFrameDatabase;

class KeyFrame
{
public:
    KeyFrame(Frame &F, Map* pMap, KeyFrameDatabase* pKFDB);

    // Pose functions
    void SetPose(const cv::Mat &Tcw);
    cv::Mat GetPose();
    cv::Mat GetPoseInverse();
    cv::Mat GetCameraCenter();
    // cv::Mat GetStereoCenter();
    // cv::Mat GetRotaion();
    // cv::Mat GetTranslation();

    // Bag of Words Representation
    void ComputeBow();

    // Covisibility graph function, 共视图函数
    // void AddConnection(KeyFrame* pKF, const int &weight); // 添加共视关键帧
    void EraseConnection(KeyFrame* pKF); // 删除共视关键帧
    // void UpdateConnections(); // 基于当前关键帧对地图点的观测构造共视图
    void UpdateBestCovisibles(); // 基于共视图信息修改对应变量
    std::set<KeyFrame *> GetConnectedKeyFrames();
    std::vector<KeyFrame *> GetVectorCovisibleKeyFrames();
    std::vector<KeyFrame *> GetBestCovisibilityKeyFrames(const int &N);
    // std::vector<KeyFrame *> GetCovisiblesByWeight(const int &w);
    int GetWeight(KeyFrame* pKF);

    // Spanning tree functions
    void AddChild(KeyFrame* pKF);       // 添加子节点，mspChildrens的set方法
    void EraseChild(KeyFrame* pKF);     // 删除子节点
    void ChangeParent(KeyFrame* pKF);   // mpParent的set方法
    std::set<KeyFrame*> GetChilds();    // mspChildrens的get方法
    KeyFrame* GetParent();              // mpParent的get方法
    bool hasChild(KeyFrame* pKF);       // 判断mspChildrens中是否存在关键帧pKF

    // MapPoint observation functions
    void AddMapPoint(MapPoint* pMP, const size_t &idx); // 添加一个地图点
    void EraseMapPointMatch(const size_t &idx);         // 根据数组位置删除地图点
    void EraseMapPointMatch(MapPoint* pMP);             // 根据输入的地图点来删除地图点列表中的地图点，这里也需要先获得需删除地图点的序号
    void ReplaceMapPointMatch(const size_t &idx, MapPoint* pMP);    // 替换地图点
    std::set<MapPoint*> GetMapPoints();             // 获取当前帧的所有地图点，并以set结构保存，并不是每个特征点都能对应一个地图点(有些没有，有些被删除了)
    std::vector<MapPoint*> GetMapPointMatches();    // mvpMapPoints的get方法
    int TrackedMapPoints(const int &minObs);

    // KeyPoint functions
    std::vector<size_t> GetFeaturesInArea(const float &x, const float &y, const float &r) const;

    // Image
    bool IsInImage(const float &x, const float &y) const;

    // Set/Check bad flag
    void SetBadFlag();  // 执行实际物理删除操作
    bool isBad();       // mbBad的get方法


    static bool lId(KeyFrame* pKF1, KeyFrame* pKF2) {
        return pKF1->mnId < pKF2->mnId;
    }

public:

    static long unsigned int nNextId;   // 
    long unsigned int mnId;             // 当前关键帧的Id
    const long unsigned int mnFrameId;  // 

    const double mTimeStamp;

    // Grid (to speed up feature matching)
    const int mnGridCols;
    const int mnGridRows;
    const float mfGridElementWidthInv;
    const float mfGridElementHeightInv;

    // Variables used by the tracking
    long unsigned int mnTrackReferenceForFrame;

    // Variables used by the keyframe database
    long unsigned int mnLoopQuery;  // 在闭环搜索候选帧时，标记该帧被哪个关键帧pKF搜索
    int mnLoopWords;                // 在一轮闭环候选帧搜索过程中，被pKF访问的次数；因为是按照pKF的BoW Tree的叶子节点来搜索的，所以同一帧有可能出现在不同的叶子节点下，所以也可以认为是被多少不同的叶子节点记录过，所以是Words
    float mLoopScore;               // 在一轮闭环候选帧搜索过程中，与pKF关键帧的相似度得分
    long unsigned int mnRelocQuery; // 在重定位搜索候选关键帧时，标记该帧被哪个帧F搜索
    int mnRelocWords;               // 在一轮闭环候选帧搜索过程中，被帧F访问的次数
    float mRelocScore;              // 在一轮闭环候选帧搜索过程中，与帧F的相似度得分

    // Calibration parameters
    const float fx;
    const float fy;
    const float cx;
    const float cy;
    const float invfx;
    const float invfy;
    const float mbf;
    const float mb;
    const float mThDepth;

    // Number of KeyPoints
    const int N;

    // KeyPoints, stereo coordinate and descriptors (all associated by an index)
    const std::vector<cv::KeyPoint> mvKeys;
    const std::vector<cv::KeyPoint> mvKeysUn;
    const std::vector<float> mvuRight;  // negative value for monocular points
    const std::vector<float> mvDepth;   // negative value for monocular points
    const cv::Mat mDescriptors;

    // BoW
    // Bag-of-Words Vector，词袋向量
    // 用于加速特征匹配和场景识别
    DBoW2::BowVector mBowVec;
    DBoW2::FeatureVector mFeatVec;

    // Scale
    const int mnScaleLevels;
    const float mfScaleFactor;
    const float mfLogScaleFactor;
    const std::vector<float> mvScaleFactors;
    const std::vector<float> mvLevelSigma2;
    const std::vector<float> mvInvLevelSigma2;

    // Spanning Tree and Loop Edges
    bool mbFirstConnection;             // 当前关键帧是否还未加入到生成树，构造函数中初始化为true，加入生成树之后置为false
    KeyFrame* mpParent;                 // 当前关键帧在生成树中的父节点
    std::set<KeyFrame*> mspChildrens;   // 当前关键帧在生成树中的子节点列表
    std::set<KeyFrame*> mspLoopEdges;   // 和当前帧形成回环的关键帧集合

    // Pose relative to parent (this is computed when bad flag is activated)
    cv::Mat mTcp;   // 父关键帧到当前帧的位姿

    // Image bounds and calibration
    const int mnMinX;
    const int mnMaxX;
    const int mnMinY;
    const int mnMaxY;
    const cv::Mat mK;

protected:
    // SE3 Pose and camera center
    cv::Mat mTcw;   // 世界坐标系到相机坐标系的位姿变换
    cv::Mat mTwc;   // 相机坐标系到世界坐标系的位姿变换
    cv::Mat mOw;    // 世界坐标系到相机坐标系的位移

    cv::Mat mCw;    // Stereo middel point. Only for visualization

    // MapPoints associated to keyframe
    std::vector<MapPoint*> mvpMapPoints;    // 当前关键帧观测到的地图点列表，该列表从Frame类中引入，数组大小是Frame的特征点数量

    // BoW
    KeyFrameDatabase* mpKeyFrameDB;
    ORBVocabulary* mpORBvocabulary;

    // Grid over the image to speed up feature matching
    std::vector<std::vector<std::vector<size_t>>> mGrid;

    std::map<KeyFrame*, int> mConnectedKeyFrameWeights;     // 当前关键帧的共视关键帧及权重
    std::vector<KeyFrame*> mvpOrderedConnectedKeyFrames;    // 所有共视关键帧，按权重从大到小排列
    std::vector<int> mvOrderedWeights;                      // 所有共视权重，按从大到小排序

    // Bad flags
    bool mbNotErase;    // 当前帧是否具有不被删除的特权
    bool mbToBeErased;  // 当前关键帧是否被豁免过删除
    bool mbBad;         // 标记当前关键帧是否为坏帧，初值为false

    float mHalfBaseline;    // Only for visualization

    Map* mpMap;

    std::mutex mMutexPose;
    std::mutex mMutexConnections;
    std::mutex mMutexFeatures;
};

}

#endif