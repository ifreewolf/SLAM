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
    mnFuseTargetForKF(0),
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


void KeyFrame::ComputeBoW()
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

cv::Mat KeyFrame::GetTranslation()
{
    std::unique_lock<std::mutex> lock(mMutexPose);
    return mTcw.rowRange(0, 3).col(3).clone();
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


/**
 * @brief 根据3D点为所有关键帧和当前帧之间建立边,每个边有一个权重,边的权重是该关键帧与当前帧公共3D点的个数(也称为两帧之间的共视程度)
 * 更新关键帧之间的连接图
 * 1.首先获得该关键帧的所有MapPoint点,统计观测到这些3D点的每个关键帧与其它所有关键帧之间的共视程度
 *      对每一个找到的关键帧，建立一条边，边的权重是该关键帧与当前帧公共3D点的个数
 * 2.并且该权重必须大于一个阈值，如果没有超过该阈值的权重，那么就只保留权重最大的边（与其他关键帧的共视程度比较高）
 * 3. 对这些连接按照权重从大到小进行排序，以方便将来的处理
 *      更新完Covisibility图之后，如果没有初始化过，则初始化为连接权重最大的边（与其他关键帧共视程度最高的那个关键帧），类似于最大生成树
 */
void KeyFrame::UpdateConnections()
{
    // KFcounter第一个参数表示某个关键帧，第二个参数该关键帧看到了多少当前帧的地图点，也就是共视程度。
    std::map<KeyFrame*, int> KFcounter;

    // Step1 通过遍历当前帧地图点获取其与其他关键帧的共视程度，存入变量KFcounter中
    // 逻辑：先获取当前帧的所有地图点，然后通过地图点获取所有关键帧，并统计所有关键帧出现的频率
    std::vector<MapPoint*> vpMP;

    {
        std::unique_lock<std::mutex> lockMPs(mMutexFeatures);
        vpMP = mvpMapPoints;    // 当前帧的所有地图点
    }

    // 地图点被关键帧观测来统计关键帧之间的共视程度
    // 统计该地图点能够被多少关键帧观测到，与当前帧形成共视关系，统计的数量存放于 KFcounter 变量中
    for (std::vector<MapPoint*>::iterator vit = vpMP.begin(), vend = vpMP.end(); vit != vend; vit++) {
        MapPoint* pMP = *vit;   // 目前进行统计的地图点

        if (!pMP) {
            continue;
        }

        if (pMP->isBad()) {
            continue;
        }

        // 对于每一个地图点，observations记录了可以观测到该地图点的所有关键帧
        std::map<KeyFrame*, size_t> observations = pMP->GetObservations();  // 当前地图点的所有观测，也就是所有关键帧；key:KeyFrame，value:该地图点在关键帧的特征点的序号

        for (std::map<KeyFrame*, size_t>::iterator mit = observations.begin(), mend = observations.end(); mit != mend; mit++) {
            if (mit->first->mnId == mnId) { // 与当前关键帧本身不算共视
                continue;
            }
            KFcounter[mit->first]++;    //  统计当前关键帧所有的地图点的观测数量，将作为权重
        }
    }

    // 没有共视关系，直接退出
    if (KFcounter.empty()) {
        return;
    }

    // 记录最高共视程度，同时如果共视程度超过了一定阈值（如默认th=15），则建立连接关系
    int nmax = 0;   // 记录最高的共视程度
    KeyFrame* pKFmax = NULL;
    int th = 15;    // 至少有15个共视地图点才会添加共视关系

    // Step2 找到与当前关键帧共视程度超过15的关键帧，存入变量vPairs中
    std::vector<std::pair<int, KeyFrame*>> vPairs;  // first:权重；second:KeyFrame
    vPairs.reserve(KFcounter.size());
    for (std::map<KeyFrame*, int>::iterator mit = KFcounter.begin(), mend = KFcounter.end(); mit != mend; mit++) {
        if (mit->second > nmax) {
            nmax = mit->second;
            pKFmax = mit->first;
        }
        // 建立共视关系至少需要大于等于th个共视地图点
        if (mit->second >= th) {    // 共视程度超过15个地图点的帧
            vPairs.push_back(std::make_pair(mit->second, mit->first));
            (mit->first)->AddConnection(this, mit->second);
        }
    }

    // Step3 如果没有超过阈值的权重，则对权重最大的关键帧建立连接
    if (vPairs.empty()) {
        vPairs.push_back(std::make_pair(nmax, pKFmax));
        pKFmax->AddConnection(this, nmax);
    }

    // Step4 对关键帧按照共视权重排序，存入变量 mvpOrderedConnectedKeyFrames和mvpOrderedWeights中
    std::sort(vPairs.begin(), vPairs.end());    // 默认按照从小到大的顺序排序
    // 将排序后的结果分别组织成为两种数据类型
    std::list<KeyFrame*> lKFs;
    std::list<int> lWs;
    for (size_t i = 0; i < vPairs.size(); i++) {
        lKFs.push_front(vPairs[i].second);
        lWs.push_front(vPairs[i].first);
    }

    {
        std::unique_lock<std::mutex> lockCon(mMutexConnections);

        // 更新当前帧与其它关键帧的连接权重
        mConnectedKeyFrameWeights = KFcounter;
        mvpOrderedConnectedKeyFrames = std::vector<KeyFrame*>(lKFs.begin(), lKFs.end());
        mvOrderedWeights = std::vector<int>(lWs.begin(), lWs.end());

        // Step5 更新生成树的连接
        // 对于第一次加入生成树的关键帧，取共视程度最高的关键帧为父关键帧
        // 该操作会改变当前关键帧的成员变量mpParent和父关键帧的成员变量mspChildrens
        if (mbFirstConnection && mnId != 0) {
            // 初始化该关键帧的父关键帧为共视程度最高的那个关键帧
            mpParent = mvpOrderedConnectedKeyFrames.front();
            // 建立双向连接关系，将当前关键帧作为其子关键帧
            mpParent->AddChild(this);
            mbFirstConnection = false;
        }
    }
}


MapPoint* KeyFrame::GetMapPoint(const size_t &idx)
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    return mvpMapPoints[idx];
}


void KeyFrame::AddConnection(KeyFrame* pKF, const int &weight)
{
    // Step1 修改变量 mConnectedKeyFrameWeights
    {
        std::unique_lock<std::mutex> lock(mMutexConnections);
        if (!mConnectedKeyFrameWeights.count(pKF)) {    // 如果不存在，则添加
            mConnectedKeyFrameWeights[pKF] = weight;
        } else if (mConnectedKeyFrameWeights[pKF] != weight) {  // 如果存在，但权重不一样，则更新权重
            mConnectedKeyFrameWeights[pKF] = weight;
        } else {
            return;
        }
    }

    // Step2. 调用函数UpdateBestCovisibles()修改变量mvpOrderedConnectedKeyFrames和mvpOrderedWeights，重新排序，目的是：当添加一个关键帧或者更新其权重时，对其顺序进行调整
    UpdateBestCovisibles();
}


/**
 * @brief 将指定的特征点映射回世界坐标系下的地图点
 * @param[in] i 特征点在关键帧中的序号
 */
cv::Mat KeyFrame::UnprojectStereo(int i)
{
    // 提取特征点的深度值
    const float z = mvDepth[i];
    if (z > 0) {
        // 提取特征点的像素坐标
        const float u = mvKeys[i].pt.x;
        const float v = mvKeys[i].pt.y;
        // 通过内参将像素坐标映射到相机坐标
        const float x = (u - cx) * z * invfx;
        const float y = (v - cy) * z * invfy;
        // 将x,y,z封装成3D点
        cv::Mat x3Dc = (cv::Mat_<float>(3, 1) << x, y, z);

        std::unique_lock<std::mutex> lock(mMutexPose);
        // 通过Twc将坐标系下的3D点坐标转换到世界坐标系下
        return mTwc.rowRange(0, 3).colRange(0, 3) * x3Dc + mTwc.rowRange(0, 3).col(3);

    } else {
        return cv::Mat();
    }
}


/**
 * 
 */
float KeyFrame::ComputeSceneMedianDepth(const int q)
{
    std::vector<MapPoint*> vpMapPoints;
    cv::Mat Tcw_;
    {
        std::unique_lock<std::mutex> lock(mMutexFeatures);
        std::unique_lock<std::mutex> lock2(mMutexPose);
        vpMapPoints = mvpMapPoints;
        Tcw_ = mTcw.clone();
    }

    std::vector<float> vDepths;
    vDepths.reserve(N);
    cv::Mat Rcw2 = Tcw_.row(2).colRange(0, 3);  // 最后一行
    Rcw2 = Rcw2.t();
    float zcw = Tcw_.at<float>(2, 3);   // z
    for (int i = 0; i < N; i++) {
        if (mvpMapPoints[i]) {
            MapPoint* pMP = mvpMapPoints[i];
            cv::Mat x3Dw = pMP->GetWorldPos();
            float z = Rcw2.dot(x3Dw) + zcw;
            vDepths.push_back(z);
        }
    }

    std::sort(vDepths.begin(), vDepths.end());

    return vDepths[(vDepths.size() - 1) / q];
}
}