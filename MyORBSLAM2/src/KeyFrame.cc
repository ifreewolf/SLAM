#include "KeyFrame.h"

namespace ORB_SLAM2
{

long unsigned int KeyFrame::nNextId = 0;

KeyFrame::KeyFrame(Frame &F, Map* pMap, KeyFrameDatabase* pKFDB):
    mnFrameId(F.mnId),
    mTimeStamp(F.mTimeStamp),
    mnGridCols(FRAME_GRID_COLS),
    mnGridRows(FRAME_GRID_ROWS),
    mfGridElementWidthInv(F.mfGridElementWidthInv),
    mfGridElementHeightInv(F.mfGridElementHeightInv),
    mnTrackReferenceForFrame(0),
    fx(F.fx),
    fy(F.fy),
    cx(F.cx),
    cy(F.cy),
    invfx(F.invfx),
    invfy(F.invfy),
    mbf(F.mbf),
    mb(F.mb),
    mThDepth(F.mThDepth),
    N(F.N),
    mvKeys(F.mvKeys),
    mvKeysUn(F.mvKeysUn),
    mvuRight(F.mvuRight),
    mvDepth(F.mvDepth),
    mDescriptors(F.mDescriptors.clone()),
    mBowVec(F.mBowVec),
    mFeatVec(F.mFeatVec),
    mvpMapPoints(F.mvpMapPoints),
    mnScaleLevels(F.mnScaleLevels),
    mfScaleFactor(F.mfScaleFactor),
    mfLogScaleFactor(F.mfLogScaleFactor),
    mvScaleFactors(F.mvScaleFactors),
    mvLevelSigma2(F.mvLevelSigma2),
    mvInvLevelSigma2(F.mvInvLevelSigma2),
    mnMinX(F.mnMinX),
    mnMaxX(F.mnMaxX),
    mnMinY(F.mnMinY),
    mnMaxY(F.mnMaxY),
    mK(F.mK),
    mpKeyFrameDB(pKFDB),
    mpORBvocabulary(F.mpORBvocabulary),
    mHalfBaseline(F.mb/2),
    mpMap(pMap)
{
    mnId = nNextId++;

    mGrid.resize(mnGridCols);
    for (int i = 0; i < mnGridCols; i++) {
        mGrid[i].resize(mnGridRows);
        for (int j = 0; j < mnGridRows; j++) {
            mGrid[i][j] = F.mGrid[i][j];
        }
    }

    SetPose(F.mTcw);
}


void KeyFrame::AddMapPoint(MapPoint* pMP, const size_t &idx)
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    mvpMapPoints[idx] = pMP;
}


bool KeyFrame::isBad()
{
    std::unique_lock<std::mutex> lock(mMutexConnections);
    return mbBad;
}


void KeyFrame::EraseConnection(KeyFrame* pKF)
{
    // Step1 修改变量 mConnectedKeyFrameWeights
    bool bUpdate = false;
    {
        std::unique_lock<std::mutex> lock(mMutexConnections);
        if (mConnectedKeyFrameWeights.count(pKF)) { // 如果当前存在该关键帧，则删除关键帧，并在后续更新共视图
            mConnectedKeyFrameWeights.erase(pKF);
            bUpdate = true;
        }
    }

    // Step2 调用函数 UpdateBestCovisibles() 修改变量 mvpOrderedConnectedKeyFrames 和 mvpOrderedWeights
    if (bUpdate) {
        UpdateBestCovisibles();
    }
}


void KeyFrame::UpdateBestCovisibles()
{
    std::unique_lock<std::mutex> lock(mMutexConnections);
    // 取出所有关键帧进行排序，排序结果存入变量 mvpOrderedConnectedKeyFrames 和 mvpOrderedWeights 中
    std::vector<std::pair<int, KeyFrame*>> vPairs;
    vPairs.reserve(mConnectedKeyFrameWeights.size());
    for (std::map<KeyFrame*, int>::iterator mit = mConnectedKeyFrameWeights.begin(), mend = mConnectedKeyFrameWeights.end(); mit != mend; mit++) {
        vPairs.push_back(std::make_pair(mit->second, mit->first));
    }

    std::sort(vPairs.begin(), vPairs.end());    // 默认从小到大排序，所以后续插入到lKFs和lWs中是头插法
    std::list<KeyFrame*> lKFs;
    std::list<int> lWs;
    for (size_t i = 0, iend = vPairs.size(); i < iend; i++) {
        lKFs.push_front(vPairs[i].second);
        lWs.push_front(vPairs[i].first);
    }

    mvpOrderedConnectedKeyFrames = std::vector<KeyFrame*>(lKFs.begin(), lKFs.end());
    mvOrderedWeights = std::vector<int>(lWs.begin(), lWs.end());
}


std::vector<KeyFrame*> KeyFrame::GetVectorCovisibleKeyFrames()
{
    std::unique_lock<std::mutex> lock(mMutexConnections);
    return mvpOrderedConnectedKeyFrames;
}


int KeyFrame::GetWeight(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lock(mMutexConnections);
    if (mConnectedKeyFrameWeights.count(pKF)) {
        return mConnectedKeyFrameWeights[pKF];
    } else {
        return 0;
    }
}


void KeyFrame::ChangeParent(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lockCon(mMutexConnections);
    mpParent = pKF;
    pKF->AddChild(this);
}


void KeyFrame::EraseChild(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lockCon(mMutexConnections);
    mspChildrens.erase(pKF);
}


cv::Mat KeyFrame::GetPose()
{
    std::unique_lock<std::mutex> lock(mMutexPose);
    return mTcw.clone();
}


cv::Mat KeyFrame::GetPoseInverse()
{
    std::unique_lock<std::mutex> lock(mMutexPose);
    return mTwc.clone();
}


void KeyFrame::EraseMapPointMatch(const size_t &idx)
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    mvpMapPoints[idx] = static_cast<MapPoint*>(NULL);
}

void KeyFrame::EraseMapPointMatch(MapPoint* pMP)
{
    int idx = pMP->GetIndexInKeyFrame(this);    // 返回的是pMP地图点在this关键帧的序号，根据地图点的共视关键帧mObservations，保存了该地图点在关键帧中的序号
    if (idx >= 0) {
        mvpMapPoints[idx] = static_cast<MapPoint*>(NULL);
    }
}


void KeyFrame::AddChild(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lock(mMutexConnections);
    mspChildrens.insert(pKF);
}


cv::Mat KeyFrame::GetCameraCenter()
{
    std::unique_lock<std::mutex> lock(mMutexPose);
    return mOw.clone();
}


void KeyFrame::SetPose(const cv::Mat &Tcw)
{
    std::unique_lock<std::mutex> lock(mMutexPose);
    Tcw.copyTo(mTcw);
    cv::Mat Rcw = Tcw.rowRange(0, 3).colRange(0, 3);    // 世界坐标系到当前帧的旋转位姿
    cv::Mat tcw = Tcw.rowRange(0, 3).col(3);            // 世界坐标系到当前帧的位移
    cv::Mat Rwc = Rcw.t();                              // 当前帧到世界坐标系的旋转位姿
    mOw = -Rwc*tcw;                                     // ？？？？？？？？？？？？？？

    mTwc = cv::Mat::eye(4, 4, mTcw.type());
    Rwc.copyTo(mTwc.rowRange(0, 3).colRange(0, 3));
    mOw.copyTo(mTwc.rowRange(0, 3).col(3));
    cv::Mat center = (cv::Mat_<float>(4, 1) << mHalfBaseline, 0, 0, 1);
    mCw = mTwc * center;
}


/**
 * @brief 被 minObs 以上关键帧(相机)观测到，认为该关键帧被追踪到
 */
int KeyFrame::TrackedMapPoints(const int &minObs)
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);

    int nPoints = 0;
    const bool bCheckObs = minObs > 0;
    for (int i = 0; i < N; i++) {
        MapPoint* pMP = mvpMapPoints[i];
        if (pMP) {
            if (!pMP->isBad()) {    // 依次检查该地图点物理上和逻辑上是否删除，若删除了就不对其进行操作
                if (bCheckObs) {
                    if (mvpMapPoints[i]->Observations() >= minObs) {
                        nPoints++;
                    }
                } else {
                    nPoints++;
                }
            }
        }
    }
    return nPoints;
}


std::vector<KeyFrame *> KeyFrame::GetBestCovisibilityKeyFrames(const int &N)
{
    std::unique_lock<std::mutex> lock(mMutexConnections);
    if ((int)mvpOrderedConnectedKeyFrames.size() < N) {
        return mvpOrderedConnectedKeyFrames;
    } else {
        return std::vector<KeyFrame*>(mvpOrderedConnectedKeyFrames.begin(), mvpOrderedConnectedKeyFrames.begin() + N);
    }
}


std::set<KeyFrame *> KeyFrame::GetConnectedKeyFrames()
{
    std::unique_lock<std::mutex> lock(mMutexConnections);
    std::set<KeyFrame*> s;
    for (std::map<KeyFrame*, int>::iterator mit = mConnectedKeyFrameWeights.begin(); mit != mConnectedKeyFrameWeights.end(); mit++) {
        s.insert(mit->first);
    }
    return s;
}


void KeyFrame::ReplaceMapPointMatch(const size_t &idx, MapPoint* pMP)
{
    mvpMapPoints[idx] = pMP;
}


std::set<MapPoint*> KeyFrame::GetMapPoints()
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    std::set<MapPoint*> s;
    for (size_t i = 0, iend = mvpMapPoints.size(); i < iend; i++) {
        if (!mvpMapPoints[i]) {
            continue;
        }
        MapPoint* pMP = mvpMapPoints[i];
        if (!pMP->isBad()) {
            s.insert(pMP);
        }
    }
    return s;
}


std::vector<MapPoint*> KeyFrame::GetMapPointMatches()
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    return mvpMapPoints;
}


std::vector<size_t> KeyFrame::GetFeaturesInArea(const float &x, const float &y, const float &r) const
{
    std::vector<size_t> vIndices;
    vIndices.reserve(N);

    const int nMinCellX = std::max(0, (int)std::floor((x - mnMinX - r) * mfGridElementWidthInv));
    if (nMinCellX >= mnGridCols) {
        return vIndices;
    }

    const int nMaxCellX = std::min((int)mnGridCols - 1, (int)std::ceil((x - mnMinX + r) * mfGridElementWidthInv));
    if (nMaxCellX < 0) {
        return vIndices;
    }

    const int nMinCellY = max(0,(int)floor((y-mnMinY-r)*mfGridElementHeightInv));
    if(nMinCellY>=mnGridRows) {
        return vIndices;
    }

    const int nMaxCellY = min((int)mnGridRows-1,(int)ceil((y-mnMinY+r)*mfGridElementHeightInv));
    if(nMaxCellY<0) {
        return vIndices;
    }

    for (int ix = nMinCellX; ix <= nMaxCellX; ix++) {
        for (int iy = nMinCellY; iy <= nMaxCellY; iy++) {
            const std::vector<size_t> vCell = mGrid[ix][iy];
            if (vCell.empty()) {
                continue;
            }
            for (size_t j = 0, jend = vCell.size(); j < jend; j++) {
                const cv::KeyPoint &kpUn = mvKeysUn[vCell[j]];
                const float distx = kpUn.pt.x - x;
                const float disty = kpUn.pt.y - y;

                if (std::fabs(distx) < r && std::fabs(disty) < r) {
                    vIndices.push_back(vCell[j]);
                }
            }
        }
    }

    return vIndices;
}


bool KeyFrame::IsInImage(const float &x, const float &y) const
{
    return (x >= mnMinX && x < mnMaxX && y >= mnMinY && y < mnMaxY);
}


std::set<KeyFrame*> KeyFrame::GetChilds()
{
    std::unique_lock<std::mutex> lockCon(mMutexConnections);
    return mspChildrens;
}


KeyFrame* KeyFrame::GetParent()
{
    std::unique_lock<std::mutex> lockCon(mMutexConnections);
    return mpParent;
}


bool KeyFrame::hasChild(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lockCon(mMutexConnections);
    return mspChildrens.count(pKF);
}


void KeyFrame::ComputeBow()
{
    if (mBowVec.empty() || mFeatVec.empty()) {
        std::vector<cv::Mat> vCurrentDesc = Converter::toDescriptorVector(mDescriptors);
        mpORBvocabulary->transform(vCurrentDesc, mBowVec, mFeatVec, 4);
    }
}


cv::Mat KeyFrame::GetRotation()
{
    std::unique_lock<std::mutex> lock(mMutexPose);
    return mTcw.rowRange(0, 3).colRange(0, 3).clone();
}


void KeyFrame::SetNotErase()    // 在LoopClosing线程调用，表示该关键帧参与回环检测
{
    std::unique_lock<std::mutex> lock(mMutexConnections);
    mbNotErase = true;  // 获得删除豁免权
}


void KeyFrame::SetErase()
{
    {
        std::unique_lock<std::mutex> lock(mMutexConnections);
        if (mspLoopEdges.empty()) { // 和当前帧形成回环的关键帧集合
            mbNotErase = false; // 剥夺删除豁免权
        }
    }

    if (mbToBeErased) { // 如果被豁免过删除，则立即删除
        SetBadFlag();
    }

    // 在这里会发现一个有意思的事，mbToBeErased只有在调用SetBadFlag()方法后才有可能被置为true，但是SetBadFlag()只有在mbToBeErased为true时才有可能被调用
    // mbToBeErased是一个protected变量，但SetBadFlag()方法是一个public。在LocalMapping中的KeyFrameCulling()方法中会被调用
    // SetErase()方法在LoopClosing中的DetectLoop()、ComputeSim3()两个方法中被调用
}


void KeyFrame::SetBadFlag()
{
    // Step1 特殊情况：豁免第一帧和具有mbNotErase特权的帧
    {
        std::unique_lock<std::mutex> lock(mMutexConnections);
        if (mnId == 0) {
            return;
        } else if (mbNotErase) {    // 如果有豁免权直接退出
            mbToBeErased = true;    // mbToBeErased标记当前KeyFrame是否被豁免过删除，LoopClosing线程不再需要某关键帧时，会调用SetErase()剥夺关键帧的
                                    // 不被删除的特权，将成员变量mnNotErase复位为false。同时检查成员变量mbToBeErased，若为true，则调用函数SetBadFlag()删除该帧
            return;
        }
    }

    // 如果没有删除豁免权，则进行两步删除：先逻辑删除，再物理删除
    // Step2 从共视关键帧的共视图中删除当前关键帧
    for (std::map<KeyFrame*, int>::iterator mit = mConnectedKeyFrameWeights.begin(), mend = mConnectedKeyFrameWeights.end(); mit != mend; mit++) {
        mit->first->EraseConnection(this);
    }

    // Step3 删除当前关键帧中地图点对当前帧的观测
    for (size_t i = 0; i < mvpMapPoints.size(); i++) {  // 当前关键帧的所有地图点删除当前帧的观测
        if (mvpMapPoints[i]) {
            mvpMapPoints[i]->EraseObservation(this);
        }
    }

    {
        // Step4 删除共视图
        std::unique_lock<std::mutex> lock(mMutexConnections);
        std::unique_lock<std::mutex> lock1(mMutexFeatures);
        mConnectedKeyFrameWeights.clear();
        mvpOrderedConnectedKeyFrames.clear();

        // Step5 更新生成树结构
        std::set<KeyFrame*> sParentCandidates;
        sParentCandidates.insert(mpParent); // 将当前关键帧的父节点加入到父节点候选区

        while (!mspChildrens.empty()) {
            bool bContinue = false;

            int maxW = -1;
            KeyFrame* pC;
            KeyFrame* pP;

            // 遍历每个子节点与候选父节点之间的权重，选择权重最大的子节点和备选父节点之间建立父子关系
            for (std::set<KeyFrame*>::iterator sit = mspChildrens.begin(), send = mspChildrens.end(); sit != send; sit++) {
                KeyFrame* pKF = *sit;   // 子节点
                if (pKF->isBad()) { // 如果被逻辑删除，则跳过
                    continue;
                }

                // Check if a parent candidate is connected to the keyframe
                std::vector<KeyFrame*> vpConnected = pKF->GetVectorCovisibleKeyFrames();    // 返回当前子节点的所有共视关键帧
                // 迭代子节点的共视关键帧
                for (size_t i = 0, iend = vpConnected.size(); i < iend; i++) {
                    // 迭代父节点候选点
                    for (std::set<KeyFrame*>::iterator spcit = sParentCandidates.begin(), spcend = sParentCandidates.end(); spcit != spcend; spcit++) {
                        // 如果当前关键帧的子节点的共视关键帧 等于 父节点候选帧，这里是要求子节点父节点必须得是子节点的共视关键帧
                        if (vpConnected[i]->mnId == (*spcit)->mnId) {   // mnId相等应该是表示他们是同一帧才对吧？？？？？？？？？？
                            int w = pKF->GetWeight(vpConnected[i]); // 获取子节点与父节点候选关键帧之间的权重
                            if (w > maxW) {
                                pC = pKF;
                                pP = vpConnected[i];
                                maxW = w;
                                bContinue = true;
                            }
                        }
                    }
                }
            }

            if (bContinue) {    // 如果找到了更大权重的关键帧
                pC->ChangeParent(pP);   // 将pP作为pC的父节点
                sParentCandidates.insert(pC);   // 将pC加入到父节点候选关键帧，将已经建立连接关系的子节点加入父节点候选关键帧中
                mspChildrens.erase(pC);         // 将pC从当前帧的子节点中删除，意味着该子节点已经找到了合适的关键帧
            } else {
                break;
            }
        }

        // If a children has no covisibility links wih any parent candidate, assign to the original parent of this KF
        if (!mspChildrens.empty()) {    // 如果还剩下子节点没找到合适的父节点，则将当前关键帧的父节点作为子节点的父节点
            for (std::set<KeyFrame*>::iterator sit = mspChildrens.begin(); sit != mspChildrens.end(); sit++) {
                (*sit)->ChangeParent(mpParent);
            }
        }

        mpParent->EraseChild(this); // 断开父节点与当前关键帧的连接
        mTcp = mTcw * mpParent->GetPoseInverse();    // 更新位姿
        // Step6 将当前关键帧 mbBad 置为 true
        mbBad = true;
    }

    // Step7 从地图中删除当前关键帧
    mpMap->EraseKeyFrame(this);
    mpKeyFrameDB->erase(this);
}


void KeyFrame::AddLoopEdge(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lockCon(mMutexConnections);
    mbNotErase = true;
    mspLoopEdges.insert(pKF);
}


std::set<KeyFrame*> KeyFrame::GetLoopEdges()
{
    std::unique_lock<std::mutex> lockCon(mMutexConnections);
    return mspLoopEdges;
}

}