#include "MapPoint.h"

namespace ORB_SLAM2
{

long unsigned int  MapPoint::nNextId = 0;
std::mutex MapPoint::mGlobalMutex;

MapPoint::MapPoint(const cv::Mat &Pos, KeyFrame* pRefKF, Map* pMap):
    mnFirstKFid(pRefKF->mnId),
    mnFirstFrame(pRefKF->mnFrameId),
    nObs(0),
    mnTrackReferenceForFrame(0),
    mnLastFrameSeen(0),
    mnBAGlobalForKF(0),
    mnFuseCandidateForKF(0),
    mnLoopPointForKF(0),
    mnCorrectedByKF(0),
    mnCorrectedReference(0),
    mpRefKF(pRefKF),
    mnVisible(1),
    mnFound(1),
    mbBad(false),
    mpReplaced(static_cast<MapPoint*>(NULL)),
    mfMinDistance(0),
    mfMaxDistance(0),
    mpMap(pMap)
{
    Pos.copyTo(mWorldPos);
    mNormalVector = cv::Mat::zeros(3, 1, CV_32F);

    // MapPoints can be created from Tracking and Local Mapping. This mutex avoid conflicts with id.
    std::unique_lock<std::mutex> lock(mpMap->mMutexPointCreation);  // 临界锁用于避免多个线程创建MapPoint时，出现相同mnId的情况
    mnId = nNextId++;
}


MapPoint::MapPoint(const cv::Mat &Pos, Map* pMap, Frame* pFrame, const int &idxF):
    mnFirstKFid(-1),
    mnFirstFrame(pFrame->mnId),
    nObs(0),
    mnTrackReferenceForFrame(0),
    mnLastFrameSeen(0),
    mnBALocalForKF(0),
    mnFuseCandidateForKF(0),
    mnLoopPointForKF(0),
    mnCorrectedByKF(0),
    mnCorrectedReference(0),
    mnBAGlobalForKF(0),
    mpRefKF(static_cast<KeyFrame*>(NULL)),
    mnVisible(1),
    mnFound(1),
    mbBad(false),
    mpReplaced(NULL),
    mpMap(pMap)
{
    Pos.copyTo(mWorldPos);
    cv::Mat Ow = pFrame->GetCameraCenter();
    mNormalVector = mWorldPos - Ow;
    mNormalVector = mNormalVector / cv::norm(mNormalVector);

    cv::Mat PC = Pos - Ow;
    const float dist = cv::norm(PC);
    const int level = pFrame->mvKeysUn[idxF].octave;
    const float levelScaleFactor = pFrame->mvScaleFactors[level];
    const int nLevels = pFrame->mnScaleLevels;

    mfMaxDistance = dist * levelScaleFactor;
    mfMinDistance = mfMaxDistance / pFrame->mvScaleFactors[nLevels - 1];

    pFrame->mDescriptors.row(idxF).copyTo(mDescriptor);

    // MapPoints can be created from Tracking and Local Mapping. This mutex avoid conflicts with id.
    std::unique_lock<std::mutex> lock(mpMap->mMutexPointCreation);
    mnId = nNextId++;

}


void MapPoint::SetWorldPos(const cv::Mat &Pos)
{
    std::unique_lock<std::mutex> lock2(mGlobalMutex);
    std::unique_lock<std::mutex> lock(mMutexPos);
    Pos.copyTo(mWorldPos);
}


cv::Mat MapPoint::GetWorldPos()
{
    std::unique_lock<std::mutex> lock(mMutexPos);
    return mWorldPos.clone();
}

void MapPoint::AddObservation(KeyFrame* pKF, size_t idx)
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    // 如果已经添加过观测，返回
    if (mObservations.count(pKF)) {
        return;
    }
    // 如果没有添加过观测，记录下能观测到该MapPoint的KF和该MapPoint在KF中的索引
    mObservations[pKF] = idx;

    // 根据观测形式是单目还是双目更新观测计数变量nObs
    if (pKF->mvuRight[idx] >= 0) {  // 双目
        nObs += 2;
    } else {    // 单目
        nObs++;
    }
}

// 从参考帧pKF中移除本地图点
void MapPoint::EraseObservation(KeyFrame* pKF)
{
    bool bBad = false;
    {
        std::unique_lock<std::mutex> lock(mMutexFeatures);
        // 查找这个要删除的观测，根据单目和双目类型的不同从其中删除当前地图点的被观测次数
        if (mObservations.count(pKF)) {
            int idx = mObservations[pKF];
            if (pKF->mvuRight[idx] >= 0) {
                nObs -= 2;
            } else {
                nObs--;
            }

            mObservations.erase(pKF);

            // 如果该KeyFrame是参考帧，该Frame被删除后重新指定RefFrame
            if (mpRefKF == pKF) {
                mpRefKF = mObservations.begin()->first;  // 选择最早加入的观察帧作为参考帧
            }

            // 当观测到该点的相机数目少于2时，丢弃该点（至少需要两个观测才能三角化）
            if (nObs <= 2) {
                bBad = true;
            }
        }
    }

    if (bBad) {
        SetBadFlag();
    }

}


std::map<KeyFrame*, size_t> MapPoint::GetObservations()
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    return mObservations;
}


bool MapPoint::IsInKeyFrame(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    return mObservations.count(pKF);
}


bool MapPoint::GetIndexInKeyFrame(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    if (mObservations.count(pKF)) {
        return mObservations[pKF];
    } else {
        return -1;
    }
}


/**
 * @brief 计算地图点最具有代表性的描述子
 * 由于一个地图点会被许多相机观测到，因此在插入关键帧后，需要判断是否更新代表当前点的描述子
 * 先获得当前点的所有描述子，然后计算描述子之间的两两距离，最好的描述子与其他描述子应该具有最小的距离中值
 */
void MapPoint::ComputeDistinctiveDescriptors()
{
    // Retrieve all observed descriptors
    std::vector<cv::Mat> vDescriptors;

    std::map<KeyFrame*, size_t> observations;

    // Step1 获取该地图点所有有效的观测关键帧信息
    {
        std::unique_lock<std::mutex> lock1(mMutexFeatures);
        if (mbBad) {
            return;
        }
        observations = mObservations;   // 观测到该MapPoint的KF和该MapPoint在KF中的索引
    }

    if (observations.empty()) {
        return;
    }

    vDescriptors.reserve(observations.size());

    // Step2 遍历观测到该地图点的所有关键帧，对应的orb描述子，放到向量vDescriptors中
    for (std::map<KeyFrame*, size_t>::iterator mit = observations.begin(), mend = observations.end(); mit != mend; mit++) {
        // mit->first  取观测到该地图点的关键帧
        // mit->second 取该地图点在关键帧中的索引
        KeyFrame* pKF = mit->first;

        if (!pKF->isBad()) {
            // 取对应的描述子向量
            vDescriptors.push_back(pKF->mDescriptors.row(mit->second));
        }
    }

    if (vDescriptors.empty()) {
        return;
    }

    // Compute distance between them
    // Step3 计算这些描述子两两之间的距离
    // N表示一共多少个描述子
    const size_t N = vDescriptors.size();

    // 将Distances表述成一个对称的矩阵
    float Distances[N][N];  // 距离矩阵，存储所有特征点描述子之间的距离，大小是描述子数量
    for (size_t i = 0; i < N; i++) {
        // 和自己的距离为0
        Distances[i][i] = 0;
        // 计算并记录不同描述子距离
        for (size_t j = i + 1; j < N; j++) {
            int distij = ORBmatcher::DescriptorDistance(vDescriptors[i], vDescriptors[j]);
            Distances[i][j] = distij;
            Distances[j][i] = distij;
        }
    }

    // Take the descriptor with least median distance to the rest
    // Step4 选择最有代表性的描述子，它与其他描述子应该具有最小的距离中值
    int BestMedian = INT_MAX;   // 记录最小的中值
    int BestIdx    = 0;         // 最小中值对应的索引
    for (size_t i = 0; i < N; i++) {    // 循环排序每个描述子到其他描述子之间的距离，并获得每个描述子距离其他描述子的中值距离
        std::vector<int> vDists(Distances[i], Distances[i] + N);
        std::sort(vDists.begin(), vDists.end());

        // 获得中值
        int median = vDists[0.5*(N - 1)];
        // 寻找最小的中值
        if (median < BestMedian) {  // 最终要选择最小的中值距离
            BestMedian = median;
            BestIdx = i;    // 记录索引
        }
    }

    {
        std::unique_lock<std::mutex> lock(mMutexFeatures);
        mDescriptor = vDescriptors[BestIdx].clone();        // 最小中值距离的描述子即为最具有代表性的描述子
    }
}


/**
 * @brief 更新地图点的平均观测方向、观测距离范围
 */
void MapPoint::UpdateNormalAndDepth()
{
    // Step1 获得观测到该地图点的所有关键帧、坐标等信息
    std::map<KeyFrame*, size_t> observations;
    KeyFrame* pRefKF;
    cv::Mat Pos;
    {
        std::unique_lock<std::mutex> lock1(mMutexFeatures);
        std::unique_lock<std::mutex> lock2(mMutexPos);
        if (mbBad) {
            return;
        }

        observations = mObservations;       // 获得观测到该地图点的所有关键帧
        pRefKF       = mpRefKF;             // 观测到该点的参考帧（第一次创建时的关键帧）
        Pos          = mWorldPos.clone();   // 地图点在世界坐标系中的坐标
    }

    if (observations.empty()) {
        return;
    }

    // Step2 根据观测到当前地图点的关键帧取平均观测方向
    // 能观测到该地图点的所有关键帧，对该点的观测方向归一化为单位向量，然后进行求和得到该地图点的朝向
    // 初始值为0向量，累加为归一化向量，最后除以总数n
    cv::Mat normal = cv::Mat::zeros(3, 1, CV_32F);
    int n = 0;
    for (std::map<KeyFrame*, size_t>::iterator mit = observations.begin(), mend = observations.end(); mit != mend; mit++) {
        KeyFrame* pKF = mit->first;
        cv::Mat Owi = pKF->GetCameraCenter();   // 观测帧在世界坐标系下的坐标
        // 获得地图点和观测到它关键帧的向量并归一化, mWorldPos是当前地图点在世界坐标系下的坐标
        cv::Mat normali = mWorldPos - Owi;
        normal = normal + normali / cv::norm(normali);  // 单位化后的观测向量做累加
        n++;
    }

    // Step3 根据参考帧计算平均观测距离
    cv::Mat PC = Pos - pRefKF->GetCameraCenter();                       // 参考关键帧相机指向地图点的向量（在世界坐标系下的表示）
    const float dist = cv::norm(PC);                                    // 该点到参考关键帧相机的距离
    const int level = pRefKF->mvKeysUn[observations[pRefKF]].octave;    // 观测到该地图点的参考帧的特征点在金字塔的第几层
    const float levelScaleFactor = pRefKF->mvScaleFactors[level];       // 当前金字塔对应的尺度因子，
    const int nLevels = pRefKF->mnScaleLevels;                          // 金字塔总层数，默认为8

    {
        // mfMaxDistance = dist * 1.2^level
        // mfMinDistance = mfMaxDistance / 1.2^7
        std::unique_lock<std::mutex> lock3(mMutexPos);
        mfMaxDistance = dist * levelScaleFactor;                                // 观测到该点的距离上限
        // std::cout << "fangguanshou level = " << level << ", levelScaleFactor = " << levelScaleFactor << ", dist = " << dist << ", mfMaxDistance = " << mfMaxDistance << std::endl;
        mfMinDistance = mfMaxDistance / pRefKF->mvScaleFactors[nLevels - 1];    // 观测到该点的距离下限
        mNormalVector = normal / n;                                             // 获得地图点平均的观测方向
    }
}


cv::Mat MapPoint::GetNormal()
{
    std::unique_lock<std::mutex> lock(mMutexPos);
    return mNormalVector.clone();
}


KeyFrame* MapPoint::GetReferenceKeyFrame()
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    return mpRefKF;
}


int MapPoint::Observations()
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    return nObs;
}


float MapPoint::GetMinDistanceInvariance()
{
    std::unique_lock<std::mutex> lock(mMutexPos);
    return 0.8f*mfMinDistance;
}


float MapPoint::GetMaxDistanceInvariance()
{
    std::unique_lock<std::mutex> lock(mMutexPos);
    return 1.2f*mfMaxDistance;
}


int MapPoint::PredictScale(const float &currentDist, KeyFrame* pKF)
{
    /**
     * UpdateNormalAndDepth()方法中计算mfMinDistance和mfMaxDistance的逆运算
     * mfMinDistance = mfMaxDistance / 2^7
     * mfMaxDistance = dist * 2^level
     * 这里是根据某地图点到某帧的观测深度估计其在该帧金字塔上的层级
     * currentDist/mfMaxDistance = 1.2^level
     * level = log1.2(currentDist/mfMaxDistance) 对结果向上取整
     */
    float ratio;
    {
        std::unique_lock<std::mutex> lock(mMutexPos);
        ratio = mfMaxDistance / currentDist;
    }

    // 对数换底公式：logb(a)=logc(a)/logc(b)
    int nScale = std::ceil(std::log(ratio) / pKF->mfLogScaleFactor);    // mfLogScaleFactor=log(1.2), log1.2(a)=log10(a)/log10(1.2)
    if (nScale < 0) {
        nScale = 0;
    } else if (nScale >= pKF->mnScaleLevels) {
        nScale = pKF->mnScaleLevels - 1;
    }
    return nScale;
}


// 原理同PredictScale(const float &currentDist, KeyFrame* pKF)
int MapPoint::PredictScale(const float &currentDist, Frame* pF)
{
    float ratio;
    {
        std::unique_lock<std::mutex> lock(mMutexPos);
        ratio = mfMaxDistance / currentDist;
    }
    int nScale = std::ceil(std::log(ratio)/pF->mfLogScaleFactor);
    if (nScale < 0) {
        nScale = 0;
    } else if (nScale >= pF->mnScaleLevels) {
        nScale = pF->mnScaleLevels - 1;
    }
    return nScale;
}


void MapPoint::SetBadFlag()
{
    std::map<KeyFrame*, size_t> obs;
    {
        std::unique_lock<std::mutex> lock1(mMutexFeatures);
        std::unique_lock<std::mutex> lock2(mMutexPos);
        mbBad = true;   // 标记mbBad，逻辑上删除当前地图点
        obs = mObservations;
        mObservations.clear();
    }
    // 删除关键帧对当前地图点的观测
    // 这里obs是当前地图点的所有观测帧，及观测帧对应的位置；要删除关键帧对当前地图点的观测，就得逐个关键帧删除
    for (std::map<KeyFrame*, size_t>::iterator mit = obs.begin(), mend = obs.end(); mit != mend; mit++) {
        KeyFrame* pKF = mit->first;
        pKF->EraseMapPointMatch(mit->second);
    }
    // 在地图类上注册删除当前地图点，这里会发生内存泄漏
    mpMap->EraseMapPoint(this);
}


bool MapPoint::isBad()
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    std::unique_lock<std::mutex> lock2(mMutexPos);
    return mbBad;
}


/**
 * @brief 用pMP这个地图点代替当前地图点
 * 逐个遍历当前地图点的观察帧；
 *  如果观察帧不是要替换的地图点的观察帧，则添加对应关系(对要替换的地图点添加观察帧的观察，对观察帧添加对该地图点的观察)
 *  如果观察帧是要替换的地图点的观察帧，则将该观察帧对该地图点的观察进行删除，因为要替换的地图点不在该帧的观察范围，所以需要删除
 */
void MapPoint::Replace(MapPoint* pMP)
{
    // 如果是同一个地图点则跳过
    if (pMP->mnId == this->mnId) {
        return;
    }

    // Step1 逻辑上删除当前地图点
    int nvisible, nfound;
    std::map<KeyFrame*, size_t> obs;
    {
        std::unique_lock<std::mutex> lock1(mMutexFeatures);
        std::unique_lock<std::mutex> lock2(mMutexPos);
        obs = mObservations;     // 备份当前地图点的观察帧
        mObservations.clear();  // 当前地图点的观察帧清零
        mbBad = true;
        nvisible = mnVisible;   // 备份状态
        nfound = mnFound;
        mpReplaced = pMP;
    }

    // Step2 将当前地图点的数据叠加到新地图上
    for (std::map<KeyFrame*, size_t>::iterator mit = obs.begin(), mend = obs.end(); mit != mend; mit++) {
        // Replace measurement in keyframe
        KeyFrame* pKF = mit->first; // 当前地图点的观察帧

        if (!pMP->IsInKeyFrame(pKF)) {  // 判断当前地图点的观察帧pKF是否是地图点pMP的观测帧
            // 如果地图点的观测帧没有该关键帧，就将该地图点替换为pKF对应位置的地图点
            // mit->second是观测帧pKF对应的地图点的序号
            pKF->ReplaceMapPointMatch(mit->second, pMP);    // 相当于 pKF->mvpMapPoints[mit->second] = pMP;
            pMP->AddObservation(pKF, mit->second);          // 将pKF添加为pMP的观测帧
        } else {
            // 如果当前地图点的观察帧是要替换的地图点的观察帧， mvpMapPoints[idx] = static_cast<MapPoint*>(NULL);
            pKF->EraseMapPointMatch(mit->second);   // 如果关键帧pKF是地图点pMP的观测帧，则删除该地图点？？？？？？？？？？？？？为什么要删除
                                                    // 回答：因为是替换，用pMP替换当前地图点，pMP不在该帧的观察范围，必然要删除，前面的操作也只是改变了当前地图点和观察帧的关系到pMP上

        }
    }

    pMP->IncreaseFound(nfound);
    pMP->IncreaseVisible(nvisible);
    pMP->ComputeDistinctiveDescriptors();

    // Step3 删除当前地图点
    mpMap->EraseMapPoint(this); // 从地图上删除当前该地图点
}


MapPoint* MapPoint::GetReplaced()
{
    std::unique_lock<std::mutex> lock1(mMutexFeatures);
    std::unique_lock<std::mutex> lock2(mMutexPos);
    return mpReplaced;
}


void MapPoint::IncreaseVisible(int n)
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    mnVisible += n;
}


void MapPoint::IncreaseFound(int n)
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    mnFound += n;
}


float MapPoint::GetFoundRatio()
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    return static_cast<float>(mnFound) / mnVisible;
}


cv::Mat MapPoint::GetDescriptor()
{
    std::unique_lock<std::mutex> lock(mMutexFeatures);
    return mDescriptor.clone();
}

}