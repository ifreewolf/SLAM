#include "ORBmatcher.h"

namespace ORB_SLAM2
{

const int ORBmatcher::TH_HIGH = 100;
const int ORBmatcher::TH_LOW = 50;
const int ORBmatcher::HISTO_LENGTH = 30;


ORBmatcher::ORBmatcher(float nnratio, bool checkOri):
    mfNNratio(nnratio),
    mbCheckOrientation(checkOri)
{

}


// Bit set count operation from
// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
int ORBmatcher::DescriptorDistance(const cv::Mat &a, const cv::Mat &b)
{
    const int *pa = a.ptr<int32_t>();
    const int *pb = b.ptr<int32_t>();

    int dist = 0;

    for (int i = 0; i < 8; i++, pa++, pb++) {
        unsigned int v = *pa ^ *pb;
        v = v - ((v >> 1) & 0x55555555);
        v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
        dist += (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
    }

    return dist;
}


/**
 * @brief 输入一张关键帧pKF，以及一张普通帧F，然后获取匹配特征点对应的地图点vpMapPointMatches
 *          通过词袋，对关键帧的特征点进行跟踪
 * 步骤：
 * Step1 分别取出属于同一node的ORB特征点（只有属于同一node，才有可能是匹配点）
 * Step2 遍历KF中属于该node的特征点
 * Step3 遍历F中属于该node的特征点，寻找最佳匹配点
 * Step4 根据阈值和角度投票剔除误匹配
 * Step5 根据方向剔除误匹配的点
 * @param[in] pKF   关键帧
 * @param[in] F     当前普通帧
 * @param[in & out] vpMapPointMatches   F中地图点对应的匹配，NULl表示未匹配
 * @return 成功匹配的数据
 * 
 * Frame是当前帧，pKF是候选关键帧。从候选关键帧中为当前帧匹配
 */
int ORBmatcher::SearchByBoW(KeyFrame *pKF, Frame &F, std::vector<MapPoint*> &vpMapPointMatches)
{
    // 获取该关键帧的地图点
    const std::vector<MapPoint*> vpMapPointsKF = pKF->GetMapPointMatches();
    
    // 匹配结果，和普通帧F特征点的索引一致，表示当前帧F对应特征点是否匹配上地图点，未匹配上是NULL
    vpMapPointMatches = std::vector<MapPoint*>(F.N, static_cast<MapPoint*>(NULL));

    // 取出关键帧的词袋特征向量
    const DBoW2::FeatureVector &vFeatVecKF = pKF->mFeatVec;

    int nmatches = 0;

    // 特征点角度旋转差统计用的直方图
    std::vector<int> rotHist[HISTO_LENGTH];
    for (int i = 0; i < HISTO_LENGTH; i++) {
        rotHist[i].reserve(500);
    }

    // 将0°~360°转换到0~HISTO_LENGTH的系数
    const float factor = HISTO_LENGTH / 360.0f;

    // 将属于同一节点（特定层）的ORB特征进行匹配
    DBoW2::FeatureVector::const_iterator KFit = vFeatVecKF.begin();
    DBoW2::FeatureVector::const_iterator Fit = F.mFeatVec.begin();
    DBoW2::FeatureVector::const_iterator KFend = vFeatVecKF.end();
    DBoW2::FeatureVector::const_iterator Fend = F.mFeatVec.end();

    while (KFit != KFend && Fit != Fend) {
        // Step1 分别取出属于同一node的ORB特征点（只有属于同一node，才有可能是匹配点）
        // first元素就是node id，遍历
        if (KFit->first == Fit->first) {
            // second是该node内存储的feature index
            const std::vector<unsigned int> vIndicesKF = KFit->second;
            const std::vector<unsigned int> vIndicesF = Fit->second;

            // Step2 遍历KF中属于该node的特征点
            for (size_t iKF = 0; iKF < vIndicesKF.size(); iKF++) {
                // 关键帧该node节点特征点的索引
                const unsigned int realIdxKF = vIndicesKF[iKF];

                // 取出KF中该特征点对应的地图点
                MapPoint* pMP = vpMapPointsKF[realIdxKF];

                if (!pMP) {
                    continue;
                }

                if (pMP->isBad()) {
                    continue;
                }

                // 取出KF中该特征点对应的描述子
                const cv::Mat &dKF = pKF->mDescriptors.row(realIdxKF);

                int bestDist1 = 256;    // 最好的距离（最小距离）
                int bestIdxF  = -1;     // 最好距离的索引
                int bestDist2 = 256;    // 次好距离（倒数第二小距离）

                // Step3 遍历F中属于该node的特征点，寻找最佳匹配点
                for (size_t iF = 0; iF < vIndicesF.size(); iF++) {
                    // 这里的realIdxF是指当前帧该节点中特征点的索引
                    const unsigned int realIdxF = vIndicesF[iF];

                    // 如果地图点存在，说明这个点已经被匹配过了，不再匹配，加快速度
                    if (vpMapPointMatches[realIdxF]) {
                        continue;
                    }

                    // 取出F中该特征点对应的描述子
                    const cv::Mat &dF = F.mDescriptors.row(realIdxF);

                    // 计算描述子的距离
                    const int dist = DescriptorDistance(dKF, dF);

                    // 遍历，记录最佳距离、最佳距离对应的索引、次佳距离等
                    // 如果 dist < bestDist1 < bestDist2, 更新bestDist1 bestDist2
                    if (dist < bestDist1) {
                        bestDist2 = bestDist1;
                        bestDist1 = dist;
                        bestIdxF = realIdxF;
                    } else if (dist < bestDist2) {  // 如果bestDist1 < dist < bestDist2，更新bestDist2
                        bestDist2 = dist;
                    }
                }

                // Step4 根据阈值 和 角度投票剔除误匹配
                // Step4.1 第一关筛选：匹配距离必须小于设定阈值
                if (bestDist1 < TH_LOW) {
                    // Step4.2 第二关筛选：最佳匹配比次佳匹配明显更好，那么最佳匹配才真正靠谱
                    if (static_cast<float>(bestDist1) < mfNNratio * static_cast<float>(bestDist2)) {
                        // Step4.3 记录成功匹配特征点的对应的地图点（来自关键帧）
                        vpMapPointMatches[bestIdxF] = pMP;

                        // 这里的realIdxKF是当前遍历到的关键帧的特征点id，获取特征点，矫正过的
                        const cv::KeyPoint &kp = pKF->mvKeysUn[realIdxKF];

                        // Step4.4 计算匹配点旋转角度差所在的直方图
                        if (mbCheckOrientation) {
                            // angle:每个特征点在提取描述子时的旋转主方向角度，如果图像旋转了，这个角度将发生改变
                            // 所有的特征点的角度变化是一致的，通过直方图统计得到最准确的角度变化值
                            float rot = kp.angle - F.mvKeys[bestIdxF].angle;
                            if (rot < 0.0) {
                                rot += 360.0f;
                            }
                            // 将rot分配到bin组，四舍五入，其实就是离散到对应的直方图组中
                            int bin = std::round(rot * factor);
                            if (bin == HISTO_LENGTH) {
                                bin = 0;
                            }
                            assert(bin >= 0 && bin < HISTO_LENGTH);
                            rotHist[bin].push_back(bestIdxF);   // 直方图统计
                        }
                        nmatches++;
                    }
                }
            }
            KFit++;
            Fit++;
        } else if (KFit->first < Fit->first) {
            // 对齐
            KFit = vFeatVecKF.lower_bound(Fit->first);
        } else {
            // 对齐
            Fit = F.mFeatVec.lower_bound(KFit->first);
        }
    }
    
    // Step5 根据方向剔除误匹配的点
    if (mbCheckOrientation) {
        // index
        int ind1 = -1;
        int ind2 = -1;
        int ind3 = -1;

        // 筛选出在旋转角度差落在直方图区间内数量最多的前三个bin的索引
        ComputeThreeMaxima(rotHist, HISTO_LENGTH, ind1, ind2, ind3);

        for (int i = 0; i < HISTO_LENGTH; i++) {
            // 如果特征点的旋转角度变化量属于这三个组，则保留
            if (i == ind1 || i == ind2 || i == ind3) {
                continue;
            }
            // 剔除掉不在前三的匹配点，因为他们不符合“主流旋转方向”
            for (size_t j = 0, jend = rotHist[i].size(); j < jend; j++) {
                vpMapPointMatches[rotHist[i][j]] = static_cast<MapPoint*>(NULL);
                nmatches--;
            }
        }
    }

    return nmatches;
}


int ORBmatcher::SearchByBoW(KeyFrame *pKF1, KeyFrame *pKF2, std::vector<MapPoint*> &vpMatches12)
{
    const std::vector<cv::KeyPoint> &vKeysUn1 = pKF1->mvKeysUn;
    const DBoW2::FeatureVector &vFeatVec1 = pKF1->mFeatVec;
    const std::vector<MapPoint*> vpMapPoints1 = pKF1->GetMapPointMatches();
    const cv::Mat &Descriptors1 = pKF1->mDescriptors;

    const std::vector<cv::KeyPoint> &vKeysUn2 = pKF2->mvKeysUn;
    const DBoW2::FeatureVector &vFeatVec2 = pKF2->mFeatVec;
    const std::vector<MapPoint*> vpMapPoints2 = pKF2->GetMapPointMatches();
    const cv::Mat &Descriptors2 = pKF2->mDescriptors;

    vpMatches12 = std::vector<MapPoint*>(vpMapPoints1.size(), static_cast<MapPoint*>(NULL));    // 分配pKF1中地图点数量的空间
    std::vector<bool> vbMatched2(vpMapPoints2.size(), false);   // 分配pKF2中地图点数量的空间

    std::vector<int> rotHist[HISTO_LENGTH];
    for (int i = 0; i < HISTO_LENGTH; i++) {
        rotHist[i].reserve(500);
    }

    const float factor = 1.0f / HISTO_LENGTH;

    int nmatches = 0;

    DBoW2::FeatureVector::const_iterator f1it = vFeatVec1.begin();
    DBoW2::FeatureVector::const_iterator f2it = vFeatVec2.begin();
    DBoW2::FeatureVector::const_iterator f1end = vFeatVec1.end();
    DBoW2::FeatureVector::const_iterator f2end = vFeatVec2.end();

    while (f1it != f1end && f2it != f2end) {
        if (f1it->first == f2it->first) {   // 叶子节点的所属节点相同
            for (size_t i1 = 0, iend1 = f1it->second.size(); i1 < iend1; i1++) {    // 遍历同一个所属节点下，pKF1中的所有特征点
                const size_t idx1 = f1it->second[i1];

                MapPoint* pMP1 = vpMapPoints1[idx1];
                if (pMP1) {
                    continue;
                }
                if (pMP1->isBad()) {
                    continue;
                }
                
                const cv::Mat &d1 = Descriptors1.row(idx1);

                int bestDist1 = 256;
                int bestIdx2 = -1;
                int bestDist2 = 256;

                for (size_t i2 = 0, iend2 = f2it->second.size(); i2 < iend2; i2++) {
                    const size_t idx2 = f2it->second[i2];

                    MapPoint* pMP2 = vpMapPoints2[idx2];

                    if (vbMatched2[idx2] || !pMP2) {
                        continue;
                    }

                    if (pMP2->isBad()) {
                        continue;
                    }

                    const cv::Mat &d2 = Descriptors2.row(idx2);

                    int dist = DescriptorDistance(d1, d2);

                    if (dist < bestDist1) {
                        bestDist2 = bestDist1;
                        bestDist1 = dist;
                        bestIdx2 = idx2;
                    } else if (dist < bestDist2) {
                        bestDist2 = dist;
                    }
                }

                if (bestDist1 < TH_LOW) {
                    if (static_cast<float>(bestDist1) < mfNNratio * static_cast<float>(bestDist2)) {
                        vpMatches12[idx1] = vpMapPoints2[bestIdx2];
                        vbMatched2[bestIdx2] = true;

                        if (mbCheckOrientation) {
                            float rot = vKeysUn1[idx1].angle -  vKeysUn2[bestDist2].angle;
                            if (rot < 0.0) {
                                rot += 360.0f;
                            }
                            int bin = std::round(rot * factor);
                            if (bin == HISTO_LENGTH) {
                                bin = 0;
                            }
                            assert(bin >= 0 && bin < HISTO_LENGTH);
                            rotHist[bin].push_back(idx1);
                        }
                        nmatches++;
                    }
                }
            }
            f1it++;
            f2it++;
        } else if (f1it->first < f2it->first) {
            f1it = vFeatVec1.lower_bound(f2it->first);
        } else {
            f2it = vFeatVec2.lower_bound(f1it->first);
        }
    }

    if (mbCheckOrientation) {
        int ind1 = -1;
        int ind2 = -1;
        int ind3 = -1;

        ComputeThreeMaxima(rotHist, HISTO_LENGTH, ind1, ind2, ind3);

        for (int i = 0; i < HISTO_LENGTH; i++) {
            if (i == ind1 || i == ind2 || i == ind3) {
                continue;
            }
            for (size_t j = 0, jend = rotHist[i].size(); j < jend; j++) {
                vpMatches12[rotHist[i][j]] = static_cast<MapPoint*>(NULL);
                nmatches--;
            }
        }
    }

    return nmatches;
}


/**
 * @brief
 * @param[in] F
 * @param[in] vpMapPoints 已经匹配好的地图点
 * @param[in] th 阈值
 * @return int 匹配上的地图点个数
 */
int ORBmatcher::SearchByProjection(Frame &F, const std::vector<MapPoint*> &vpMapPoints, const float th)
{
    int nmatches = 0;

    const bool bFactor = th != 1.0;

    for (size_t iMP = 0; iMP < vpMapPoints.size(); iMP++) {
        MapPoint* pMP = vpMapPoints[iMP];
        if (!pMP->mbTrackInView) {
            continue;
        }

        if (pMP->isBad()) {
            continue;
        }

        const int &nPredictedLevel = pMP->mnTrackScaleLevel;

        // The size of the window will depend on the viewing direction, cos(观测角度)
        float r = RadiusByViewingCos(pMP->mTrackViewCos);

        if (bFactor) {
            r *= th;
        }

        const std::vector<size_t> vIndices = F.GetFeaturesInArea(pMP->mTrackProjX, pMP->mTrackProjY, r * F.mvScaleFactors[nPredictedLevel], nPredictedLevel - 1, nPredictedLevel);

        if (vIndices.empty()) {
            continue;
        }

        const cv::Mat MPdescriptor = pMP->GetDescriptor();

        int bestDist = 256;
        int bestLevel = -1;
        int bestDist2 = 256;
        int bestLevel2 = -1;
        int bestIdx = -1;

        // Get best and second matches with near keypoints
        for (std::vector<size_t>::const_iterator vit = vIndices.begin(), vend = vIndices.end(); vit != vend; vit++) {
            const size_t idx = *vit;

            if (F.mvpMapPoints[idx]) {
                if (F.mvpMapPoints[idx]->Observations() > 0) {
                    continue;
                }
            }

            if (F.mvuRight[idx] > 0) {
                const float er = std::fabs(pMP->mTrackProjXR - F.mvuRight[idx]);
                if (er > r*F.mvScaleFactors[nPredictedLevel]) {
                    continue;
                }
            }

            const cv::Mat &d = F.mDescriptors.row(idx);

            const int dist = DescriptorDistance(MPdescriptor, d);

            if (dist < bestDist) {
                bestDist2 = bestDist;
                bestDist = dist;
                bestLevel2 = bestLevel;
                bestLevel = F.mvKeysUn[idx].octave;
                bestIdx = idx;
            } else if (dist < bestDist2) {
                bestLevel2 = F.mvKeysUn[idx].octave;
                bestDist2 = dist;
            }
        }

        // Apply ratio to second match (only if best and second are in the same scale level)
        if (bestDist <= TH_HIGH) {
            if (bestLevel == bestLevel2 && bestDist > mfNNratio * bestDist2) {
                continue;
            }
            F.mvpMapPoints[bestIdx] = pMP;
            nmatches++;
        }
    }

    return nmatches;
}


/**
 * @brief 将上一帧跟踪的地图点投影到当前帧，并且搜索匹配点，用于跟踪前一帧
 * 步骤
 * Step1 建立旋转直方图，用于检测旋转一致性
 * Step2 计算当前帧和前一帧的平移向量
 * Step3 对于前一帧的每一个地图点，通过相机投影模型，得到投影到当前帧的像素坐标
 * Step4 根据相机的前后前进方向来判断搜索尺度范围
 * Step5 遍历候选匹配点，寻找距离最小的最佳匹配点
 * Step6 计算匹配点旋转角度差所在的直方图
 * Step7 进行旋转一致检测，剔除不一致的匹配
 * @param[in] CurrentFrame  当前帧
 * @param[in] LastFrame     上一帧
 * @param[in] th            搜索范围阈值，默认单目为7，双目为15
 * @param[in] bMono         是否为单目
 * @return int              成功匹配的数量
 */
int ORBmatcher::SearchByProjection(Frame &CurrentFrame, const Frame &LastFrame, const float th, const bool bMono)
{
    int nmatches = 0;

    // Step1 建立旋转直方图，用于检测旋转一致性
    std::vector<int> rotHist[HISTO_LENGTH]; // HISTO_LENGTH = 30
    for (int i = 0; i < HISTO_LENGTH; i++) {
        rotHist[i].reserve(500);
    }

    const float factor = HISTO_LENGTH / 360.0f; // 30

    // Step2 计算当前帧和前一帧的平移向量
    // 当前帧的相机位姿
    const cv::Mat Rcw = CurrentFrame.mTcw.rowRange(0, 3).colRange(0, 3);
    const cv::Mat tcw = CurrentFrame.mTcw.rowRange(0, 3).col(3);

    // 当前相机坐标系到世界坐标系的平移向量
    const cv::Mat twc = -Rcw.t() * tcw; // 这个应该是相机光心在世界坐标系中的坐标

    // 上一帧的相机位姿
    const cv::Mat Rlw = LastFrame.mTcw.rowRange(0, 3).colRange(0, 3);
    const cv::Mat tlw = LastFrame.mTcw.rowRange(0, 3).col(3);

    // 当前帧相对于上一帧相机的平移向量
    const cv::Mat tlc = Rlw*twc + tlw;  // 当前帧相机光心在上一帧相机坐标系中的坐标

    // 判断前进还是后退
    const bool bForward = tlc.at<float>(2) > CurrentFrame.mb && !bMono;     // 非单目情况，如果z大于基线，则表示相机明显前进
    const bool bBackward = -tlc.at<float>(2) > CurrentFrame.mb && !bMono;   // 非单目情况，如果-z大于基线，则表示相机明显后退

    // Step3 对于前一帧的每一个地图点，通过相机投影模型，得到投影到当前帧的像素坐标
    for (int i = 0; i < LastFrame.N; i++) {
        MapPoint* pMP = LastFrame.mvpMapPoints[i];
        if (pMP) {
            if (!LastFrame.mvbOutlier[i]) {
                // 对上一帧有效的MapPoints投影到当前帧坐标系
                cv::Mat x3Dw = pMP->GetWorldPos();  // 世界坐标系下的地图点坐标
                cv::Mat x3Dc = Rcw*x3Dw + tcw;      // 当前相机坐标系下的地图点坐标，这里将上一帧的地图点投影到当前帧坐标系下

                const float xc = x3Dc.at<float>(0);
                const float yc = x3Dc.at<float>(1);
                const float invzc = 1.0 / x3Dc.at<float>(2);

                if (invzc < 0) {
                    continue;
                }

                // 投影到当前帧中
                // [u]   |fx 0 cx|[x]
                // [v] = |0 fy cy|[y]
                // [/]   |0  0  1|[z]
                float u = CurrentFrame.fx * xc * invzc + CurrentFrame.cx;
                float v = CurrentFrame.fy * yc * invzc + CurrentFrame.cy;

                if (u < CurrentFrame.mnMinX || u > CurrentFrame.mnMaxX) {
                    continue;
                }
                if (v < CurrentFrame.mnMinY || v > CurrentFrame.mnMaxY) {
                    continue;
                }

                // 上一帧中地图点对应二维特征点所在的金字塔层级
                int nLastOctave = LastFrame.mvKeys[i].octave;

                // Search in a window. Size depends on scale
                // 单目：th = 7, 双目：th = 15
                float radius = th * CurrentFrame.mvScaleFactors[nLastOctave];   // 尺度越大，搜索范围越大

                // 记录候选匹配点的id
                std::vector<size_t> vIndices2;

                // Step4 根据相机得前后前进方向来判断搜索尺度范围
                // 以下可以这么理解，例如一个有一定面积的圆点，在某个尺度n下它是一个特征点
                // 当相机前进时，圆点的面积增大，在某个尺度m下它是一个特征点，由于面积增大，则需要在更高的尺度下才能检测出来
                // 当相机后退时，圆点的面积减小，在某个尺度m下它是一个特征点，由于面积减小，则需要在更低的尺度下才能检测出来
                // GetFeaturesInArea()函数的目的是根据坐标点(u,v)和radius获得当前帧圆范围内所有特征点索引
                if (bForward) { // 前进，则上一帧兴趣点在所在得尺度 nLastOctave <= nCurOctave
                    vIndices2 = CurrentFrame.GetFeaturesInArea(u, v, radius, nLastOctave);
                } else if (bBackward) { //  后退，则上一帧兴趣点在所在的尺度 0 <= nCurOctave <= nLastOctave
                    vIndices2 = CurrentFrame.GetFeaturesInArea(u, v, radius, 0, nLastOctave);
                } else {    // 在[nLastOctave-1, nLastOctave + 1]中搜索
                    vIndices2 = CurrentFrame.GetFeaturesInArea(u, v, nLastOctave - 1, nLastOctave + 1);
                }

                if (vIndices2.empty()) {
                    continue;
                }

                const cv::Mat dMP = pMP->GetDescriptor();

                int bestDist = 256;
                int bestIdx2 = -1;

                // Step5 遍历候选匹配点，寻找距离最小的最佳匹配点
                for (std::vector<size_t>::const_iterator vit = vIndices2.begin(), vend = vIndices2.end(); vit != vend; vit++) {
                    const size_t i2 = *vit;

                    // 如果该特征点已经有对应的MapPoint了，则退出该次循环
                    if (CurrentFrame.mvpMapPoints[i2]) {
                        if (CurrentFrame.mvpMapPoints[i2]->Observations() > 0) {    // 当前帧该特征点有地图点了
                            continue;
                        }
                    }

                    if (CurrentFrame.mvuRight[i2] > 0) {    // 双目/RGBD
                        // 双目和RGBD的情况,需要保证右图的点也在搜索半径以内
                        const float ur = u - CurrentFrame.mbf * invzc;
                        const float er = std::fabs(ur - CurrentFrame.mvuRight[i2]);
                        if (er > radius) {
                            continue;
                        }
                    }

                    const cv::Mat &d = CurrentFrame.mDescriptors.row(i2);

                    const int dist = DescriptorDistance(dMP, d);

                    if (dist < bestDist) {
                        bestDist = dist;
                        bestIdx2 = i2;
                    }
                }

                // 最佳匹配距离要小于设定阈值
                if (bestDist <= TH_HIGH) {  // 100
                    CurrentFrame.mvpMapPoints[bestIdx2] = pMP;
                    nmatches++;

                    // Step6 计算匹配点旋转角度差所在的直方图
                    if (mbCheckOrientation) {
                        float rot = LastFrame.mvKeysUn[i].angle - CurrentFrame.mvKeysUn[bestIdx2].angle;
                        if (rot < 0.0f) {
                            rot += 360.0f;
                        }
                        int bin = std::round(rot * factor);
                        if (bin == HISTO_LENGTH) {
                            bin = 0;
                        }
                        assert(bin >= 0 && bin < HISTO_LENGTH);
                        rotHist[bin].push_back(bestIdx2);
                    }
                }
            }
        }
    }

    // Apply rotation consistency
    // Step7 进行旋转一致性检测,剔除不一致的匹配
    if (mbCheckOrientation) {
        int ind1 = -1;
        int ind2 = -1;
        int ind3 = -1;

        ComputeThreeMaxima(rotHist, HISTO_LENGTH, ind1, ind2, ind3);

        for (int i = 0; i < HISTO_LENGTH; i++) {
            // 对于数量不是前3个的点对,剔除
            if (i != ind1 && i != ind2 && i != ind3) {
                for (size_t j = 0, jend = rotHist[i].size(); j < jend; j++) {
                    CurrentFrame.mvpMapPoints[rotHist[i][j]] = static_cast<MapPoint*>(NULL);
                    nmatches--;
                }
            }
        }
    }

    return nmatches;
}


/**
 * @brief
 * @param[in] CurrentFrame  当前帧
 * @param[in] pKF           候选关键帧
 * @param[in] sAlreadyFound 已经找到的地图点集
 * @param[in] th            确定候选特征点搜索半径相关的参数
 * @param[in] ORBdist       描述子距离（只有两个描述子之间的汉明距离少于该值才有可能成为匹配好的点对）
 * @return int              匹配上的特征点
 */
int ORBmatcher::SearchByProjection(Frame &CurrentFrame, KeyFrame* pKF, const std::set<MapPoint*> &sAlreadyFound, const float th, const int ORBdist)
{
    int nmatches = 0;

    // 当前帧的相机位姿
    const cv::Mat Rcw = CurrentFrame.mTcw.rowRange(0, 3).colRange(0, 3);
    const cv::Mat tcw = CurrentFrame.mTcw.rowRange(0, 3).col(3);

    // 当前相机坐标系到世界坐标系的平移向量，也就是世界坐标系下，当前帧相机的坐标
    const cv::Mat Ow = -Rcw.t() * tcw;

    // Rotation Histogram (to check rotation consistency)
    std::vector<int> rotHist[HISTO_LENGTH];
    for (int i = 0; i < HISTO_LENGTH; i++) {
        rotHist[i].reserve(500);
    }

    const float factor = HISTO_LENGTH / 360.0f;

    // 获取候选帧的地图点
    const std::vector<MapPoint*> vpMPs = pKF->GetMapPointMatches();

    // 遍历地图点
    for (size_t i = 0, iend = vpMPs.size(); i < iend; i++) {
        MapPoint* pMP = vpMPs[i];

        if (pMP) {
            // 地图点可用，且没有在已经找到的地图点集合中
            if (!pMP->isBad() && !sAlreadyFound.count(pMP)) {
                // Project
                // 地图点的三维坐标点
                cv::Mat x3Dw = pMP->GetWorldPos();
                // 候选地图点转换到当前帧的三维坐标系中
                cv::Mat x3Dc = Rcw*x3Dw + tcw;

                const float xc = x3Dc.at<float>(0);
                const float yc = x3Dc.at<float>(1);
                const float invzc = 1.0 / x3Dc.at<float>(2);

                // 候选帧三维地图点在当前帧图像坐标系中的坐标
                const float u = CurrentFrame.fx * xc * invzc + CurrentFrame.cx;
                const float v = CurrentFrame.fy * yc * invzc + CurrentFrame.cy;

                // 如果转换的图像坐标不在图像内，放弃掉这个地图点
                if (u < CurrentFrame.mnMinX || u > CurrentFrame.mnMaxX) {
                    continue;
                }
                if (v < CurrentFrame.mnMinY || v > CurrentFrame.mnMaxY) {
                    continue;
                }

                // Compute predicted scale level
                // 计算金字塔层级
                // 三维点 - 当前帧的坐标系原点 = 候选帧地图点指向当前帧相机原点的向量
                cv::Mat PO = x3Dw - Ow;
                float dist3D = cv::norm(PO);    // 这个向量的范数即世界坐标系上地图点到相机光心的距离

                const float maxDistance = pMP->GetMaxDistanceInvariance();
                const float minDistance = pMP->GetMinDistanceInvariance();

                // Depth must be inside the scale pyramid of the image
                if (dist3D < minDistance || dist3D > maxDistance) {
                    continue;
                }

                int nPredictedLevel = pMP->PredictScale(dist3D, &CurrentFrame); // 预测特征点所在的金字塔层级

                // Search in a window
                // 搜索范围，每个金字塔层级不一样，跟金字塔层级缩放系数有关
                const float radius = th * CurrentFrame.mvScaleFactors[nPredictedLevel];

                // 根据地图点在当前帧的映射图像坐标，radius区间范围内，搜索所有特征点；返回
                const std::vector<size_t> vIndices2 = CurrentFrame.GetFeaturesInArea(u, v, nPredictedLevel - 1, nPredictedLevel + 1);

                if (vIndices2.empty()) {
                    continue;
                }

                const cv::Mat dMP = pMP->GetDescriptor();   // 获得地图点的描述子

                int bestDist = 256;
                int bestIdx2 = -1;

                // 遍历候选特征点
                for (std::vector<size_t>::const_iterator vit = vIndices2.begin(); vit != vIndices2.end(); vit++) {
                    const size_t i2 = *vit;
                    if (CurrentFrame.mvpMapPoints[i2]) {
                        continue;
                    }

                    const cv::Mat &d = CurrentFrame.mDescriptors.row(i2);

                    const int dist = DescriptorDistance(dMP, d);    // 描述子距离

                    if (dist < bestDist) {
                        bestDist = dist;
                        bestIdx2 = i2;
                    }
                }

                // 最好的特征点与当前帧的距离，要小于ORBdist
                if (bestDist <= ORBdist) {
                    CurrentFrame.mvpMapPoints[bestIdx2] = pMP;
                    nmatches++;

                    if (mbCheckOrientation) {
                        float rot = pKF->mvKeysUn[i].angle - CurrentFrame.mvKeysUn[bestIdx2].angle;
                        if (rot < 0.0) {
                            rot += 360.0f;
                        }
                        int bin = std::round(rot * factor);
                        if (bin == HISTO_LENGTH) {
                            bin = 0;
                        }
                        assert(bin >= 0 && bin < HISTO_LENGTH);
                        rotHist[bin].push_back(bestIdx2);
                    }
                }
            }
        }
    }

    if (mbCheckOrientation) {
        int ind1 = -1;
        int ind2 = -1;
        int ind3 = -1;

        ComputeThreeMaxima(rotHist, HISTO_LENGTH, ind1, ind2, ind3);

        for (int i = 0; i < HISTO_LENGTH; i++) {
            if (i != ind1 && i != ind2 && i != ind3) {
                for (size_t j = 0, jend = rotHist[i].size(); j < jend; j++) {
                    CurrentFrame.mvpMapPoints[rotHist[i][j]] = NULL;
                    nmatches--;
                }
            }
        }
    }

    return nmatches;
}


/**
 * @param[in] vpPoints 候选地图点
 * @param[in] vpMatched 已经匹配好的地图点，里面包含部分匹配好的，也有未匹配好的
 */
int ORBmatcher::SearchByProjection(KeyFrame* pKF, cv::Mat Scw, const std::vector<MapPoint*> &vpPoints, std::vector<MapPoint*> &vpMatched, int th)
{
    // Get Calibration Parameters for later projection
    const float &fx = pKF->fx;
    const float &fy = pKF->fy;
    const float &cx = pKF->cx;
    const float &cy = pKF->cy;

    // Decompose Scw
    cv::Mat sRcw = Scw.rowRange(0, 3).colRange(0, 3);
    const float scw = std::sqrt(sRcw.row(0).dot(sRcw.row(0)));
    cv::Mat Rcw = sRcw / scw;
    cv::Mat tcw = Scw.rowRange(0, 3).col(3) / scw;
    cv::Mat Ow = -Rcw.t() * tcw;

    // Set of MapPoints already found in the KeyFrame
    std::set<MapPoint*>  spAlreadyFound(vpMatched.begin(), vpMatched.end());
    spAlreadyFound.erase(static_cast<MapPoint*>(NULL));

    int nmatches = 0;

    // For each Candidate MapPoint Project and Match
    for (int iMP = 0, iendMP = vpPoints.size(); iMP < iendMP; iMP++) {
        MapPoint* pMP = vpPoints[iMP];

        // Discard Bad MapPoints and already found
        if (pMP->isBad() || spAlreadyFound.count(pMP)) {
            continue;
        }

        // Get 3D Coords.
        cv::Mat p3Dw = pMP->GetWorldPos();  // 候选地图点在世界坐标系下的坐标

        // Transform into Camera Coords.
        cv::Mat p3Dc = Rcw*p3Dw + tcw;  // 将候选地图点的坐标，映射到参考帧坐标系下

        // Depth must be positive
        if (p3Dc.at<float>(2) < 0.0) {
            continue;
        }

        // Project into Image
        const float invz = 1.0 / p3Dc.at<float>(2);
        const float x = p3Dc.at<float>(0) * invz;
        const float y = p3Dc.at<float>(1) * invz;

        const float u = fx*x + cx;
        const float v = fy*y + cy;

        // Point must be inside the image
        if (!pKF->IsInImage(u, v)) {
            continue;
        }

        // Depth must be inside the scale invariance region of the point
        const float maxDistance = pMP->GetMaxDistanceInvariance();
        const float minDistance = pMP->GetMinDistanceInvariance();
        cv::Mat PO = p3Dw - Ow; // 地图点世界坐标系下的坐标到参考帧pKF的相机中心在世界坐标系的坐标，获得参考帧pKF相机光心指向地图点的向量
        const float dist = cv::norm(PO);    // 参考帧pKF相机光心指向地图点的距离

        if (dist < minDistance || dist > maxDistance) {
            continue;
        }

        // Viewing angle must be less than 60 deg
        cv::Mat Pn = pMP->GetNormal();  // 候选地图点

        // 向量点乘：A·B=|A||B|cos(θ)，所以cos(θ) = A·B / |A||B|，因为要求θ<60°，素以cos(θ)>cos(60°)=0.5
        // 也就是A·B / |A||B| > 0.5 ==> A·B > |A||B|*0.5
        if (PO.dot(Pn) < 0.5*dist) {    // 这里表示A·B < |A||B|*0.5，因为Pn是单位向量，所以|Pn|=1
            continue;
        }

        int nPredictedLevel = pMP->PredictScale(dist, pKF);

        // Search in a radius
        const float radius = th * pKF->mvScaleFactors[nPredictedLevel];

        const std::vector<size_t> vIndices = pKF->GetFeaturesInArea(u, v, radius);

        if (vIndices.empty()) {
            continue;
        }

        // Match to the most similar keypoint int the radius
        const cv::Mat dMP = pMP->GetDescriptor();   // 候选地图点的描述子

        int bestDist = 256;
        int bestIdx = -1;
        for (std::vector<size_t>::const_iterator vit = vIndices.begin(), vend = vIndices.end(); vit != vend; vit++) {   // 遍历pKF中的候选特征点
            const size_t idx = *vit;
            if (vpMatched[idx]) {   // 如果已经匹配好了，不重复匹配
                continue;
            }

            const int &kpLevel = pKF->mvKeysUn[idx].octave; // pKF中候选特征点所在层级

            if (kpLevel < nPredictedLevel - 1 || kpLevel > nPredictedLevel + 1) {
                continue;
            }

            const cv::Mat &dKF = pKF->mDescriptors.row(idx);    // 参考帧pKF候选特征点的描述子

            const int dist = DescriptorDistance(dMP, dKF);

            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = idx;
            }
        }

        if (bestDist <= TH_LOW) {
            vpMatched[bestIdx] = pMP;
            nmatches++;
        }
    }

    return nmatches;
}


void ORBmatcher::ComputeThreeMaxima(std::vector<int>* histo, const int L, int &ind1, int &ind2, int &ind3)
{
    int max1 = 0;
    int max2 = 0;
    int max3 = 0;

    for (int i = 0; i < L; i++) {
        const int s = histo[i].size();
        if (s > max1) {
            max3 = max2;
            max2 = max1;
            max1 = s;
            ind3 = ind2;
            ind2 = ind1;
            ind1 = i;
        } else if (s > max2) {
            max3 = max2;
            max2 = s;
            ind3 = ind2;
            ind2 = i;
        } else if (s > max3) {
            max3 = s;
            ind3 = i;
        }
    }

    if (max2 < 0.1f * (float)max1) {
        ind2 =  -1;
        ind3 = -1;
    } else if (max3 < 0.1f * (float)max1) {
        ind3 = -1;
    }
}


float ORBmatcher::RadiusByViewingCos(const float &viewCos)
{
    if (viewCos > 0.998) {
        return 2.5;
    } else {
        return 4.0;
    }
}


}