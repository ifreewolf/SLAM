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


int ORBmatcher::SearchBySim3(KeyFrame* pKF1, KeyFrame* pKF2, std::vector<MapPoint*> &vpMatches12, const float &s12, const cv::Mat &R12, const cv::Mat &t12, const float th)
{
    const float &fx = pKF1->fx;
    const float &fy = pKF1->fy;
    const float &cx = pKF1->cx;
    const float &cy = pKF1->cy;

    // Camera 1 from world
    cv::Mat R1w = pKF1->GetRotation();
    cv::Mat t1w = pKF1->GetTranslation();

    // Camera 2 from world
    cv::Mat R2w = pKF2->GetRotation();
    cv::Mat t2w = pKF2->GetTranslation();

    // Transformation between cameras
    cv::Mat sR12 = s12*R12;
    cv::Mat sR21 = (1.0/s12)*R12.t();
    cv::Mat t21 = -sR21*t12;

    const std::vector<MapPoint*> vpMapPoints1 = pKF1->GetMapPointMatches();
    const int N1 = vpMapPoints1.size();

    const std::vector<MapPoint*> vpMapPoints2 = pKF2->GetMapPointMatches();
    const int N2 = vpMapPoints2.size();

    std::vector<bool> vbAlreadyMatched1(N1, false);
    std::vector<bool> vbAlreadyMatched2(N2, false);

    for (int i = 0; i < N1; i++) {
        MapPoint* pMP = vpMatches12[i];
        if (pMP) {
            vbAlreadyMatched1[i] = true;
            int idx2 = pMP->GetIndexInKeyFrame(pKF2);
            if (idx2 >= 0 && idx2 < N2) {
                vbAlreadyMatched2[idx2] = true;
            }
        }
    }

    std::vector<int> vnMatch1(N1, -1);
    std::vector<int> vnMatch2(N2, -1);

    // Transform from KF1 to KF2 and search
    for (int i1 = 0; i1 < N1; i1++) {
        MapPoint* pMP = vpMapPoints1[i1];
        if (!pMP || vbAlreadyMatched1[i1]) {
            continue;
        }

        if (pMP->isBad()) {
            continue;
        }

        cv::Mat p3Dw = pMP->GetWorldPos();
        cv::Mat p3Dc1 = R1w*p3Dw + t1w;     // 地图点在KF1坐标系下的坐标
        cv::Mat p3Dc2 = sR21*p3Dc1 + t21;   // 地图点从KF1转换到KF2下坐标系中的坐标

        // Depth must be positive
        if (p3Dc2.at<float>(2) < 0.0) {
            continue;
        }

        const float invz = 1.0 / p3Dc2.at<float>(2);
        const float x = p3Dc2.at<float>(0) * invz;
        const float y = p3Dc2.at<float>(1) * invz;

        const float u = fx * x + cx;
        const float v = fy * y + cy;

        // Point must be inside the image
        if (!pKF2->IsInImage(u, v)) {
            continue;
        }

        const float maxDistance = pMP->GetMaxDistanceInvariance();
        const float minDistance = pMP->GetMinDistanceInvariance();
        const float dist3D = cv::norm(p3Dc2);

        // Depth must be inside the scale invariance region
        if (dist3D < minDistance || dist3D > maxDistance) {
            continue;
        }

        // Compute predicted octave
        const int nPredictedLevel = pMP->PredictScale(dist3D, pKF2);

        // Search in a radius
        const float radius = th * pKF2->mvScaleFactors[nPredictedLevel];

        const std::vector<size_t> vIndices = pKF2->GetFeaturesInArea(u, v, radius);

        if (vIndices.empty()) {
            continue;
        }

        // Match to the most similar keypoint in the radius
        const cv::Mat dMP = pMP->GetDescriptor();

        int bestDist = INT_MAX;
        int bestIdx = -1;
        for (std::vector<size_t>::const_iterator vit = vIndices.begin(), vend = vIndices.end(); vit != vend; vit++) {
            const size_t idx = *vit;

            const cv::KeyPoint &kp = pKF2->mvKeysUn[idx];

            if (kp.octave < nPredictedLevel - 1 || kp.octave > nPredictedLevel) {
                continue;
            }

            const cv::Mat &dKF = pKF2->mDescriptors.row(idx);

            const int dist = DescriptorDistance(dMP, dKF);

            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = idx;
            }
        }

        if (bestIdx <= TH_HIGH) {
            vnMatch1[i1] = bestIdx;
        }
    }

    // Transform from KF2 to KF1 and search
    for (int i2 = 0; i2 < N2; i2++) {
        MapPoint* pMP = vpMapPoints2[i2];

        if (!pMP || vbAlreadyMatched2[i2]) {
            continue;
        }

        if (pMP->isBad()) {
            continue;
        }

        cv::Mat p3Dw = pMP->GetWorldPos();
        cv::Mat p3Dc2 = R2w * p3Dw + t2w;
        cv::Mat p3Dc1 = sR12 * p3Dc2 + t12;

        // Depth must be positive
        if (p3Dc1.at<float>(2) < 0.0) {
            continue;
        }

        const float invz = 1.0 / p3Dc1.at<float>(2);
        const float x = p3Dc1.at<float>(0) * invz;
        const float y = p3Dc1.at<float>(1) * invz;

        const float u = fx * x + cx;
        const float v = fy * y + cy;

        // Point must be inside the image
        if (!pKF1->IsInImage(u, v)) {
            continue;
        }
        const float maxDistance = pMP->GetMaxDistanceInvariance();
        const float minDistance = pMP->GetMinDistanceInvariance();
        const float dist3D = cv::norm(p3Dc1);

        // Depth must be inside the scale pyramid of the image
        if (dist3D < minDistance || dist3D > maxDistance) {
            continue;
        }

        // Compute predicted octave
        const int nPredictedLevel = pMP->PredictScale(dist3D, pKF1);

        // Search in a radius of 205*sigma(ScaleLevel)
        const float radius = th * pKF1->mvScaleFactors[nPredictedLevel];

        const std::vector<size_t> vIndices = pKF1->GetFeaturesInArea(u, v, radius);

        if (vIndices.empty()) {
            continue;
        }

        // Match to the most similar keypoint in the radius
        const cv::Mat dMP = pMP->GetDescriptor();

        int bestDist = INT_MAX;
        int bestIdx = -1;
        for (std::vector<size_t>::const_iterator vit = vIndices.begin(), vend = vIndices.end(); vit != vend; vit++) {
            const size_t idx = *vit;
            const cv::KeyPoint &kp = pKF1->mvKeysUn[idx];

            if (kp.octave < nPredictedLevel - 1 || kp.octave > nPredictedLevel) {
                continue;
            }

            const cv::Mat &dKF = pKF1->mDescriptors.row(idx);

            const int dist = DescriptorDistance(dMP, dKF);

            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = idx;
            }
        }

        if (bestDist <= TH_HIGH) {
            vnMatch2[i2] = bestIdx;
        }
    }

    // Check agreement
    int nFound = 0;
    for (int i1 = 0; i1 < N1; i1++) {
        int idx2 = vnMatch1[i1];

        if (idx2 >= 0) {
            int idx1 = vnMatch2[idx2];
            if (idx1 == i1) {
                vpMatches12[i1] = vpMapPoints2[idx2];
                nFound++;
            }
        }
    }

    return nFound;
}


/**
 * @brief 将地图点投影到关键帧中进行匹配和融合，融合策略如下：
 * 1. 如果地图点能匹配关键帧的特征点，并且该点有对应的地图点，那么选择观测数目最多的替换两个地图点
 * 2. 如果地图点能匹配关键帧的地图点，并且该点没有对应的地图点，那么为该点添加该投影地图点
 * 
 * @param[in] pKF           关键帧
 * @param[in] vpMapPoints   待投影的地图点
 * @param[in] th            搜索窗口的阈值，默认为3
 * @return int              更新地图点的数量
 */
int ORBmatcher::Fuse(KeyFrame* pKF, const std::vector<MapPoint*> &vpMapPoints, const float th)
{
    // 取出当前帧位姿，内参、光心再世界坐标系下坐标
    cv::Mat Rcw = pKF->GetRotation();
    cv::Mat tcw = pKF->GetTranslation();

    const float &fx = pKF->fx;
    const float &fy = pKF->fy;
    const float &cx = pKF->cx;
    const float &cy = pKF->cy;
    const float &bf = pKF->mbf;

    cv::Mat Ow = pKF->GetCameraCenter();

    int nFused = 0;

    const int nMPs = vpMapPoints.size();

    // 遍历所有的待投影的地图点
    for (int i = 0; i < nMPs; i++) {
        MapPoint* pMP = vpMapPoints[i];
        // Step1 判断地图点的有效性
        if (!pMP) {
            continue;
        }
        // 地图点无效 或 已经是该帧的地图点(无需融合)，跳过
        if (pMP->isBad() || pMP->IsInKeyFrame(pKF)) {
            continue;
        }

        // 将地图点变换到关键帧的相机坐标系下
        cv::Mat p3Dw = pMP->GetWorldPos();
        cv::Mat p3Dc = Rcw * p3Dw + tcw;

        // 深度值为负，跳过
        if (p3Dc.at<float>(2) < 0.0f) {
            continue;
        }

        // Step2 得到地图点投影到关键帧的图像坐标
        const float invz = 1.0 / p3Dc.at<float>(2);
        const float x = p3Dc.at<float>(0) * invz;
        const float y = p3Dc.at<float>(1) * invz;

        const float u = fx * x + cx;
        const float v = fy * y + cy;

        // 投影点需要在有效范围内
        if (!pKF->IsInImage(u, v)) {
            continue;
        }

        const float ur = u - bf * invz;

        const float maxDistance = pMP->GetMaxDistanceInvariance();
        const float minDistance = pMP->GetMinDistanceInvariance();

        cv::Mat PO = p3Dw - Ow;
        const float dist3D = cv::norm(PO);

        // Step3 地图点到关键帧相机光心距离需满足在有效范围内
        if (dist3D < minDistance || dist3D > maxDistance) {
            continue;
        }

        // Step4 地图点到光心的连线与该地图点的平均观测向量之间夹角要小于60°
        cv::Mat Pn = pMP->GetNormal();
        if (PO.dot(Pn) < 0.5 * dist3D) {
            continue;
        }

        // 根据地图点到相机光心距离预测匹配点所在的金字塔尺度
        int nPredictedLevel = pMP->PredictScale(dist3D, pKF);

        // 确定搜索范围
        const float radius = th * pKF->mvScaleFactors[nPredictedLevel];
        // Step5 在投影点附近搜索窗口内找到候选匹配点的索引
        const std::vector<size_t> vIndices = pKF->GetFeaturesInArea(u, v, radius);

        if (vIndices.empty()) {
            continue;
        }

        // Step6 遍历寻找最佳匹配点
        const cv::Mat dMP = pMP->GetDescriptor();

        int bestDist = 256;
        int bestIdx = -1;
        for (std::vector<size_t>::const_iterator vit = vIndices.begin(), vend = vIndices.end(); vit != vend; vit++) {
            const size_t idx = *vit;

            const cv::KeyPoint &kp = pKF->mvKeysUn[idx];

            const int &kpLevel = kp.octave;
            // 金字塔层级要接近（同一层或小一层），否则跳过
            if (kpLevel < nPredictedLevel - 1 || kpLevel > nPredictedLevel) {
                continue;
            }

            // 计算投影点与候选匹配特征点的距离，如果偏差很大，直接跳过
            if (pKF->mvuRight[idx] >= 0) {
                // 双目情况
                const float &kpx = kp.pt.x;
                const float &kpy = kp.pt.y;
                const float &kpr = pKF->mvuRight[idx];
                const float ex = u - kpx;
                const float ey = v - kpy;
                // 右目数据的偏差也要考虑进去
                const float er = ur - kpr;
                const float e2 = ex*ex + ey*ey + er*er;
                // 自由度为3，误差小于1个像素，这种事情95%发生的概率对应卡方检验阈值为7.82
                if (e2 * pKF->mvInvLevelSigma2[kpLevel] > 7.8) {
                    continue;
                }
            } else {
                // 单目情况
                // 计算投影点与候选匹配特征点的距离，如果偏差很大，直接跳过
                const float &kpx = kp.pt.x;
                const float &kpy = kp.pt.y;
                const float ex = u - kpx;
                const float ey = v - kpy;
                const float e2 = ex*ex + ey*ey;

                // 自由度为2，卡方检验阈值5.95（假设测量有一个像素的偏差）
                if (e2 * pKF->mvInvLevelSigma2[kpLevel] > 5.99) {
                    continue;
                }
            }

            const cv::Mat &dKF = pKF->mDescriptors.row(idx);

            const int dist = DescriptorDistance(dMP, dKF);
            // 和投影点的描述子距离最小
            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = idx;
            }
        }

        // Step7 找到投影点对应的最佳匹配特征点，根据是否存在地图点来融合和新增
        if (bestDist <= TH_LOW) {
            MapPoint* pMPinKF = pKF->GetMapPoint(bestIdx);
            if (pMPinKF) {
                // 如果最佳匹配点有对应有效地图点，选择被观测次数最多的那个替换
                if (!pMPinKF->isBad()) {
                    if (pMPinKF->Observations() > pMP->Observations()) {
                        pMP->Replace(pMPinKF);
                    } else {
                        pMPinKF->Replace(pMP);
                    }
                }
            } else {
                // 如果最佳匹配点没有对应地图点，添加观测信息
                pMP->AddObservation(pKF, bestIdx);
                pKF->AddMapPoint(pMP, bestIdx);
            }
            nFused++;
        }
    }
    return nFused;
}


/**
 * @brief 闭环矫正中使用。将当前关键帧闭环匹配上的关键帧及其共视关键帧组成的地图点投影到当前关键帧，融合地图点
 * @param[in] pKF               当前关键帧，它是需要闭环的帧的相连关键帧
 * @param[in] Scw               当前关键帧经过Sim3后的世界到pKF相机坐标系的Sim变换
 * @param[in] vpPoints          与当前关键帧闭环匹配上的关键帧及其共视关键帧组成的地图点
 * @param[in] th                搜索范围系数
 * @param[out] vpReplacePoint   替换的地图点
 * @return int                  融合（替换和新增）的地图点数目
 */
int ORBmatcher::Fuse(KeyFrame* pKF, cv::Mat Scw, const std::vector<MapPoint*> &vpPoints, float th, std::vector<MapPoint*> &vpReplacePoint)
{
    // Get Calilbration Parameters for later projection
    const float &fx = pKF->fx;
    const float &fy = pKF->fy;
    const float &cx = pKF->cx;
    const float &cy = pKF->cy;

    // Step1 将Sim3转化为SE3并分解
    //       [sR t]
    // Scw = [0  1]
    // 欧式变换的逆变换是：
    //        [R^T -R^Tt]
    // T^-1 = [0       1]
    // 所以下面sRcw就是前三行三列
    // 为什么sRcw的第一行点乘可以获得尺度？？？
    // R是一个正交且行列式为1的矩阵，正交矩阵的行向量均为单位向量且两两正交‌
    // 所以sR第一行点乘结果s^2，再开平方即可得到尺度s
    cv::Mat sRcw = Scw.rowRange(0, 3).colRange(0, 3);
    const float scw = sqrt(sRcw.row(0).dot(sRcw.row(0)));   // 计算得到尺度s，
    cv::Mat Rcw = sRcw / scw;   // 除掉s，获得真正的R
    cv::Mat tcw = Scw.rowRange(0, 3).col(3) / scw;   // 除以s，在Sim3变换中，平移分量 t 是在缩放后的坐标系中定义的。为了得到真实世界坐标系中的平移量，需要除以尺度因子进行校正
    cv::Mat Ow = -Rcw.t() * tcw;    // Ow表示关键帧光心在世界坐标系下的坐标。Rcw是世界坐标系到当前帧坐标系的变换，Rcw*Ow+tcw=(0,0,0) ==> Ow = -Rcw^T*tcw
                                    // 将光心在世界坐标系下的坐标转换到当前帧坐标系下，光心坐标为(0，0，0)，所以有如上推算。

    // 当前帧已有的匹配地图点
    const std::set<MapPoint*> spAlreadyFound = pKF->GetMapPoints();

    int nFused = 0;
    // 与当前帧闭环匹配上的关键帧及其共视关键帧组成的地图点
    const int nPoints = vpPoints.size();    // 闭环候选帧组的地图点投影到当前帧pKF

    // 对闭环匹配组关键帧组的所有地图点进行遍历
    for (int iMP = 0; iMP < nPoints; iMP++) {
        MapPoint* pMP = vpPoints[iMP];

        // 判断pMP(闭环组地图点)是否为坏点，或者已经是pKF的地图点了，则跳过
        if (pMP->isBad() || spAlreadyFound.count(pMP)) {
            continue;
        }

        // Step2 地图点变换到当前坐标系下
        cv::Mat p3Dw = pMP->GetWorldPos();

        // Transform into Camera Coords.
        cv::Mat p3Dc = Rcw * p3Dw + tcw;    // Rcw是世界坐标系到pKF，p3Dw是当前地图点在世界坐标系下的坐标，因此p3Dc就是地图点在pKF下的坐标

        // 判断地图点p3Dc是否在相机前面，不是则跳过
        if (p3Dc.at<float>(2) < 0.0f) {
            continue;
        }

        // Step3 得到地图点投影到当前帧的图像坐标
        const float invz = 1.0 / p3Dc.at<float>(2);
        const float x = p3Dc.at<float>(0) * invz;
        const float y = p3Dc.at<float>(1) * invz;

        const float u = fx * x + cx;
        const float v = fy * y + cy;

        // 投影点必须在图像范围内
        if (!pKF->IsInImage(u, v)) {
            continue;
        }

        // Step4 根据距离是否在图像合理金字塔尺度范围内和观测角度是否小于60°判断该地图点是否有效
        const float maxDistance = pMP->GetMaxDistanceInvariance();
        const float minDistance = pMP->GetMinDistanceInvariance();
        cv::Mat PO = p3Dw - Ow; // p3Dw是地图点在世界坐标系下的坐标，Ow是pKF光心在世界坐标系下的坐标，因此PO就是地图点到光心的向量
        const float dist3D = cv::norm(PO);  // 取模，就是地图点到pKF的距离

        if (dist3D < minDistance || dist3D > maxDistance) {
            continue;
        }

        cv::Mat Pn = pMP->GetNormal();  // 地图点的方向

        if (PO.dot(Pn) < 0.5 * dist3D) {    // PO是地图点方向，Pn是地图点的朝向，PO*Pn=|PO|·|Pn|cos(θ) ==> cos(θ) = PO*Pn / (|PO|·|Pn|)，其中|Pn|=1，|Pn|=dist3D；
            continue;                       // θ<60°，就是cos(60°)=0.5，因此PO*Pn / (|PO|·|Pn|)>0.5 ==> PO*Pn > 0.5*dist3D 才满足要求
        }

        // Compute predicted scale level
        const int nPredictedLevel = pMP->PredictScale(dist3D, pKF);

        // 计算搜索范围
        const float radius = th * pKF->mvScaleFactors[nPredictedLevel];

        // Step5 在当前帧内搜索匹配候选点
        const std::vector<size_t> vIndices = pKF->GetFeaturesInArea(u, v, radius);

        if (vIndices.empty()) {
            continue;
        }

        // Step6 寻找最佳匹配点（没有用到次佳匹配的比例）
        const cv::Mat dMP = pMP->GetDescriptor();

        int bestDist = INT_MAX;
        int bestIdx = -1;
        for (std::vector<size_t>::const_iterator vit = vIndices.begin(); vit != vIndices.end(); vit++) {
            const size_t idx = *vit;
            const int &kpLevel = pKF->mvKeysUn[idx].octave;

            if (kpLevel < nPredictedLevel - 1 || kpLevel > nPredictedLevel) {
                continue;
            }

            const cv::Mat &dKF = pKF->mDescriptors.row(idx);
            // 计算候选特征点（来自于pKF）与pMP(闭环组地图点)BRIEF描述子的汉明距离
            int dist = DescriptorDistance(dMP, dKF);

            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = idx;
            }
        }

        // Step7 替换或新增地图点
        if (bestDist <= TH_LOW) {
            MapPoint* pMPinKF = pKF->GetMapPoint(bestIdx);
            if (pMPinKF) {
                // 如果这个地图点已经存在，则记录要替换信息
                // 这里不能直接替换，原因是需要对地图点加锁后才能替换，否则可能会crash，所以先记录，在枷锁后替换
                if (!pMPinKF->isBad()) {
                    vpReplacePoint[iMP] = pMPinKF;  // vpReplacePoint是和vpPoints对应的地图点，表示pKF上的pMPinKF地图点将要被vpPoints对应位置的地图点来替换
                }
            } else {
                // 如果这个地图点不存在，直接添加
                pMP->AddObservation(pKF, bestIdx);
                pKF->AddMapPoint(pMP, bestIdx);
            }
            nFused++;
        }
    }
    // 融合（替换和新增）的地图点数目、
    return nFused;
}


/**
 * @brief 利用基础矩阵F12极线约束,用BoW加速匹配两个关键帧的未匹配的特征点,产生新的匹配点对
 * 具体来说,pKF1图像的每个特征点与pKF2图像同一node节点的所有特征点依次匹配,判断是否满足对极几何约束,满足约束就是匹配的特征点
 * @param[in] pKF1              关键帧1
 * @param[in] pKF2              关键帧2
 * @param[in] F12               从2到1的基础矩阵
 * @param[out] vMatchedPairs    存储匹配特征点对,特征点用其在关键帧中的索引表示
 * @param[in] bOnlyStereo       在双目和RGBD情况下,是否要求特征点在右图存在匹配
 * @return                      成功匹配的数量
 */
int ORBmatcher::SearchForTriangulation(KeyFrame* pKF1, KeyFrame* pKF2, cv::Mat F12, std::vector<std::pair<size_t, size_t>> &vMatchedPairs, const bool bOnlyStereo)
{
    const DBoW2::FeatureVector &vFeatVec1 = pKF1->mFeatVec;
    const DBoW2::FeatureVector &vFeatVec2 = pKF2->mFeatVec;

    // Step1 计算KF1的相机中心在KF2图像平面的二维像素坐标
    // KF1相机光心在世界坐标系坐标C2
    cv::Mat Cw = pKF1->GetCameraCenter();
    // KF2相机位姿R2w,t2w,是世界坐标系到相机坐标系
    cv::Mat R2w = pKF2->GetRotation();
    cv::Mat t2w = pKF2->GetTranslation();
    // KF1的相机光心转化到KF2坐标系中的坐标
    cv::Mat C2 = R2w * Cw + t2w;
    const float invz = 1.0f / C2.at<float>(2);
    // 得到KF1的相机光心在KF2中的坐标,也叫极点,这里是像素坐标
    //                        |fx 0 cx||X|
    // Puv = 1/Z *(KP) = (1/Z)|0 fy cy||Y|
    //                        |0  0  1||Z|
    // u = fx*X/Z + cx
    // v = fy*Y/Z + cy
    const float ex = pKF2->fx * C2.at<float>(0) * invz + pKF2->cx;
    const float ey = pKF2->fy * C2.at<float>(1) * invz + pKF2->cy;

    int nmatches = 0;
    // 记录匹配是否成功，避免重复匹配
    std::vector<bool> vbMatched2(pKF2->N, false);
    std::vector<int> vMatches12(pKF1->N, -1);
    // 用于统计匹配点对旋转差的直方图
    std::vector<int> rotHist[HISTO_LENGTH];
    for (int i = 0; i < HISTO_LENGTH; i++) {
        rotHist[i].reserve(500);
    }

    const float factor = HISTO_LENGTH / 360.0f;

    // Step2 利用BoW加速匹配：只对属于同一节点（特定层）的ORB特征进行匹配
    // FeatureVector其实就是一个map类，那就可以直接获取它的迭代器进行匹配
    // FeatureVector的数据结构类似于：((node1, feature_vector1), (node2, feature_vector2)...)
    // flit->first对应node编号，flit->second对应属于该node的所有特征编号
    DBoW2::FeatureVector::const_iterator f1it = vFeatVec1.begin();
    DBoW2::FeatureVector::const_iterator f2it = vFeatVec2.begin();
    DBoW2::FeatureVector::const_iterator f1end = vFeatVec1.end();
    DBoW2::FeatureVector::const_iterator f2end = vFeatVec2.end();

    // Step2.1 遍历pKF1和pKF2中的node节点
    while (f1it != f1end && f2it != f2end) {
        // 如果f1it和f2it属于同一个node节点才会进行匹配，这就是BoW加速匹配原理
        if (f1it->first == f2it->first) {

            // Step2.2 遍历属于同一node节点（id::f1it->first）下的所有特征点
            for (size_t i1 = 0, iend1 = f1it->second.size(); i1 < iend1; i1++) {
                // 获取pKF1中属于该node节点的所有特征点索引
                const size_t idx1 = f1it->second[i1];

                // Step2.3 通过特征点索引idx1在pKF1中取出对应的MapPoint
                MapPoint* pMP1 = pKF1->GetMapPoint(idx1);

                // 由于寻找的是未匹配的特征点，所以pMP1应该为NULL
                if (pMP1) {
                    continue;
                }

                // 如果mvuRight中的值大于0，表示是双目，且该特征点有深度值
                const bool bStereo1 = pKF1->mvuRight[idx1] >= 0;

                if (bOnlyStereo) {  // 要求特征点在右图存在匹配
                    if (!bStereo1) {    // 是否是双目，如果不是双目，直接退出
                        continue;
                    }
                }

                // Step2.4 通过特征点索引idx在pKF1中取出对应的特征点
                const cv::KeyPoint &kp1 = pKF1->mvKeysUn[idx1];

                // 通过特征点索引idx1在pKF1中取出对应的特征点的描述子
                const cv::Mat &d1 = pKF1->mDescriptors.row(idx1);

                int bestDist = TH_LOW;
                int bestIdx2 = -1;

                // Step2.5 遍历该node节点下（f2it->first）对应KF2中的所有特征点
                for (size_t i2 = 0, iend2 = f2it->second.size(); i2 < iend2; i2++) {
                    // 获取pKF2中属于该node节点的所有特征点索引
                    size_t idx2 = f2it->second[i2];

                    // 通过特征点索引idx2在pKF2中取出对应的MapPoint
                    MapPoint* pMP2 = pKF2->GetMapPoint(idx2);

                    // 如果pKF2当前特征点索引idx2已经被匹配过或者对应的3D点非空，那么跳过这个索引idx2
                    if (vbMatched2[idx2] || pMP2) {
                        continue;
                    }

                    const bool bStereo2 = pKF2->mvuRight[idx2] >= 0;

                    if (bOnlyStereo) {
                        if (!bStereo2) {
                            continue;
                        }
                    }

                    // 通过特征点索引idx2在pKF2中取出对应的特征点的描述子
                    const cv::Mat &d2 = pKF2->mDescriptors.row(idx2);

                    // Step2.6 计算idx1与idx2在两个关键帧中对应特征点的描述子距离
                    const int dist = DescriptorDistance(d1, d2);

                    if (dist > TH_LOW || dist > bestDist) {
                        continue;
                    }

                    // 通过特征点索引idx2在pKF2中取出对应的特征点
                    const cv::KeyPoint &kp2 = pKF2->mvKeysUn[idx2];

                    // ？为什么双目就不需要判断像素点到极点的距离的判断？
                    // 因为双目模式下可以左右互匹配恢复三维点
                    if (!bStereo1 && !bStereo2) {   // 单目模式
                        const float distex = ex - kp2.pt.x; // ex，ey是KF1光心在KF2中的图像坐标
                        const float distey = ey - kp2.pt.y;
                        // Step2.7 极点e2到KP2的像素距离如果小于阈值th，认为kp2对应的MapPoint距离pKF1相机太近，跳过
                        // 作者根据KP2金字塔尺度因子(scale^n, scale=1.2, n为层数)定义阈值th
                        // 金字塔层数从0到7，对应距离 sqrt(100*pKF2->mvScaleFactors[kp2.octave])是10~20个像素
                        // ？对这个阈值的有效性持怀疑态度
                        if (distex*distex + distey*distey < 100 * pKF2->mvScaleFactors[kp2.octave]) {
                            continue;
                        }
                    }

                    // Step2.8 计算特征点kp2到kp1对应极线的距离小于阈值
                    if (CheckDistEpipolarLine(kp1, kp2, F12, pKF2)) {
                        // bestIdx2, bestDist是kp1对应KF2中的最佳匹配点 index及匹配距离
                        bestIdx2 = idx2;
                        bestDist = dist;
                    }
                }

                if (bestIdx2 >= 0) {
                    const cv::KeyPoint &kp2 = pKF2->mvKeysUn[bestIdx2];
                    // 记录匹配结果
                    vMatches12[idx1] = bestIdx2;
                    nmatches++;

                    // 记录旋转差直方图信息
                    if (mbCheckOrientation) {
                        // angle:角度，表示匹配点对的方向差
                        float rot = kp1.angle - kp2.angle;
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

    // Step3 用旋转差直方图来筛掉错误匹配对
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
                vMatches12[rotHist[i][j]] = -1;
                nmatches--;
            }
        }
    }

    // Step4 存储匹配关系，下标是关键帧1的特征点i，存储的是关键帧2的特征点id
    vMatchedPairs.clear();
    vMatchedPairs.reserve(nmatches);

    for (size_t i = 0, iend = vMatches12.size(); i < iend; i++) {
        if (vMatches12[i] < 0) {
            continue;
        }
        vMatchedPairs.push_back(std::make_pair(i, vMatches12[i]));
    }

    return nmatches;
}


/**
 * @brief 用基础矩阵检查极线距离是否符合要求
 * @param[in] kp1   KF1中特征点
 * @param[in] kp2   KF2中特征点
 * @param[in] F12   从KF1到KF2的基础矩阵
 * @param[in] pKF2  关键帧KF2
 * @return true
 * @return false
 */
bool ORBmatcher::CheckDistEpipolarLine(const cv::KeyPoint &kp1, const cv::KeyPoint &kp2, const cv::Mat &F12, const KeyFrame *pKF2)
{

    // Step1 求出kp1在pKF2上对应的极线 l = x1'F12 = [a b c]
    const float a = kp1.pt.x * F12.at<float>(0, 0) + kp1.pt.y * F12.at<float>(1, 0) + F12.at<float>(2, 0);
    const float b = kp1.pt.y * F12.at<float>(0, 1) + kp1.pt.y * F12.at<float>(1, 1) + F12.at<float>(2, 1);
    const float c = kp1.pt.x * F12.at<float>(0, 2) + kp1.pt.y * F12.at<float>(1, 2) + F12.at<float>(2, 2);

    // Step2 计算kp2特征点到极线l2的距离
    // 极线l2:ax + by + c = 0
    // (u, v)到l2的距离为：|au + bv + c| / sqrt(a^2 + b^2)
    const float num = a * kp2.pt.x + b * kp2.pt.y + c;
    const float den = a*a + b*b;

    // 距离无穷大
    if (den == 0) {
        return false;
    }

    // 距离的平方
    const float dsqr = num * num / den;

    // Step3 判断误差是否满足条件，尺度越大，误差范围应该越大
    // 金字塔底层一个像素就占一个像素，在倒数第二层，一个像素等于最底层1.2个像素(假设金字塔尺度为1.2)
    // 3.84是自由度为1时，服从高斯分布的一个平方项（也就是这里的误差）小于一个像素，这件事发生概率超过95%时的概率（卡方分布）
    return dsqr < 3.84 * pKF2->mvLevelSigma2[kp2.octave];
}

}