#include "LocalMapping.h"

namespace ORB_SLAM2
{
LocalMapping::LocalMapping(Map* pMap, const bool bMonocular):
    mbMonocular(bMonocular), mpMap(pMap), mbAbortBA(false), mbStopRequested(false), mbFinished(true),
    mbResetRequested(false)
{
    /**
     * mbStopRequested  外部线程调用，为true，表示外部线程请求停止LocalMapping线程
     * mbAbortBA        是否终止BA优化流程
     */

}


void LocalMapping::SetLoopCloser(LoopClosing* pLoopCloser)
{
    mpLoopCloser = pLoopCloser;
}


void LocalMapping::SetTracker(Tracking* pTracker)
{
    mpTracker = pTracker;
}



/**
 * @brief 线程主线程
 * 1. 首先告诉跟踪线程，目前处于繁忙状态，之前你送过来的关键帧都没有处理完，所以现在不要发送关键帧过来，等我处理完了再说；
 */
void LocalMapping::Run()
{
    // 标记状态,表示当前Run函数正在运行,尚未结束
    mbFinished = false;
    // 主循环
    while (1) {
        // Step1 告诉Tracking,LocalMapping正处于繁忙状态,不要再发送关键帧
        // LocalMapping线程处理的关键帧都是Tracking线程发来的
        SetAcceptKeyFrames(false);

        // 等待处理的关键帧列表不为空
        if (CheckNewKeyFrames()) {

            // Step2 处理列表中的关键帧，包括计算BoW、更新观测、描述子、共视图，插入到地图等
            ProcessNewKeyFrame();

            // Step3 根据地图点的观测情况剔除质量不好的地图点
            MapPointCulling();

            // Step4 当前关键帧与相连关键帧通过三角化产生新的地图点，使得跟踪更稳
            CreateNewMapPoints();

            // 已经处理完队列中的最后一个关键帧
            if (!CheckNewKeyFrames()) {
                // Step5 检查并融合当前关键帧与相邻关键帧（两级相邻）中重复的地图点
                SearchInNeighbors();
            }

            // 终止BA的标志
            mbAbortBA = false;

            // 已经处理完队列中的最后一个关键帧，并且闭环检测没有请求停止LocalMapping
            if (!CheckNewKeyFrames() && !stopRequested()) {
                // Step6 当局部地图中的关键帧大于2个的时候进行局部地图的BA
                if(mpMap->KeyFramesInMap() > 2) {
                    // 注意这里的第二个参数是按地址传递的，当这里的 mbAbortBA 状态发生变化时，能够及时执行/停止BA
                    Optimizer::LocalBundleAdjustment(mpCurrentKeyFrame, &mbAbortBA, mpMap);
                }

                // Step7 检测并剔除当前帧相邻的关键帧中冗余的关键帧
                // 冗余的判断：该关键帧的90%的地图点可以被其他关键帧观测到
                KeyFrameCulling();
            }

            // Step8 将当前帧加入到闭环检测队列中
            // 注意这里的关键帧被设置为了bad的情况，这个需要注意
            mpLoopCloser->InsertKeyFrame(mpCurrentKeyFrame);

        } else if (Stop()) {    // 当要终止当前线程的时候
            // Safe area to stop
            while (isStopped() && !CheckFinish()) {
                // 如果还没有结束,那么等待
                std::this_thread::sleep_for(std::chrono::microseconds(3000));
            }
            // 然后确定终止了就跳出这个线程的主循环
            if (CheckFinish()) {
                break;
            }
        }

        // 查看是否有复位线程的请求
        ResetIfRequested();

        // Tracking wil see that Local Mapping is busy
        SetAcceptKeyFrames(true);

        // 如果当前线程已经结束了就跳出主循环
        if (CheckFinish()) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(3000));
    }

    // 设置线程已经终止了
    SetFinish();
}


void LocalMapping::RequestStop()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    mbStopRequested = true;
    std::unique_lock<std::mutex> lock2(mMutexNewKFs);
    mbAbortBA = true; // 放弃Bundle Adjustment
}


bool LocalMapping::isStopped()
{
    std::unique_lock<std::mutex>(mMutexStop);
    return mbStopped;
}

void LocalMapping::Release()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    std::unique_lock<std::mutex> lock2(mMutexFinish);
    if (mbFinished) {   // LocalMapping线程是否已经完成
        return;
    }
    mbStopped = false;
    mbStopRequested = false;
    for (std::list<KeyFrame*>::iterator lit = mlNewKeyFrames.begin(), lend = mlNewKeyFrames.end(); lit != lend; lit++) {
        delete *lit;
    }
    mlNewKeyFrames.clear();

    std::cout << "Local Mapping RELEASE" << std::endl;
}

void LocalMapping::RequestReset()
{
    {
        std::unique_lock<std::mutex> lock(mMutexReset);
        mbResetRequested = true;
    }

    while (1) {
        {
            std::unique_lock<std::mutex> lock2(mMutexReset);
            if (!mbResetRequested) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(3000));
    }
}


void LocalMapping::InsertKeyFrame(KeyFrame* pKF)
{
    std::unique_lock<std::mutex> lock(mMutexNewKFs);
    mlNewKeyFrames.push_back(pKF);
    mbAbortBA = true;   // 放弃 Bundle Adjustment
}


bool LocalMapping::stopRequested()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    return mbStopRequested;
}


bool LocalMapping::AcceptKeyFrames()
{
    std::unique_lock<std::mutex> lock(mMutexAccept);
    return mbAcceptKeyFrames;
}


bool LocalMapping::SetNotStop(bool flag)
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    if (flag && mbStopped) {
        return false;
    }

    mbNotStop = flag;

    return true;
}


void LocalMapping::InterruptBA()
{
    mbAbortBA = true;
}


void LocalMapping::RequestFinish()
{
    std::unique_lock<std::mutex> lock(mMutexFinish);
    mbFinishRequested = true;
}


bool LocalMapping::isFinished()
{
    std::unique_lock<std::mutex> lock(mMutexFinish);
    return mbFinished;
}


void LocalMapping::SetAcceptKeyFrames(bool flag)
{
    std::unique_lock<std::mutex> lock(mMutexAccept);
    mbAcceptKeyFrames = flag;
}


bool LocalMapping::CheckNewKeyFrames()
{
    std::unique_lock<std::mutex> lock(mMutexNewKFs);
    return (!mlNewKeyFrames.empty());
}


/**
 * @brief 处理列表中的关键帧，包括计算BoW、更新观测、描述子、共视图、插入地图等
 * 1. 从mlNewKeyFrames中取出列表最前面的关键帧，随后马上把列表中的该关键帧删除，然后对该关键帧进行处理
 * 2. 利用关键帧特征点的BRIEF描述子计算BoW，主要获得两个变量：
 *      ①mBowVec->单词word的id，以及该word对应的权重
 *      ②mFeatVec->第一个元素为节点的id，第二个元素为该节点id下所有特征点在图像中的索引
 * 3. 获得关键帧的地图点，然后循环对每个地图点进行处理。如果地图点不是坏点，则进一步处理。
 * 4. 如果该地图点存在于关键帧中：
 *      ①如果地图点不是来自当前帧的观测（比如来自局部地图点），为当前地图点添加观测
 *      ②更新该点的平均观测方向河观测距离范围
 *      ③更新地图点的最佳描述子
 * 5. 如果该地图点不存在于关键帧中：如果当前帧中已经包含了这个地图点，但是这个地图点中却没有包含这个关键帧的信息，
 *      这些地图点可能来自双目或RGBD跟踪过程中新生成的地图点，或者是CreateNewMapPoints中通过三角化产生
 * 6. 更新关键帧间的连接关系（共视图），将该关键帧插入到地图中
 */
void LocalMapping::ProcessNewKeyFrame()
{
    // Step1 从缓冲队列中取出一帧关键帧
    // 该关键帧队列是Tracking线程向LocalMapping中插入的关键帧组成的
    {
        std::unique_lock<std::mutex> lock(mMutexNewKFs);
        // 取出列表中最前面的关键帧，作为当前要处理的关键帧
        mpCurrentKeyFrame = mlNewKeyFrames.front();
        // 取出最前面的关键帧后，在原来的列表里删掉该关键帧
        mlNewKeyFrames.pop_front();
    }

    // Step2 计算该关键帧特征点的词袋向量
    mpCurrentKeyFrame->ComputeBoW();

    // Step3 处理当前关键帧中有效的地图点，更新Normal，描述子等信息
    // TrackLocalMap中和当前帧新匹配上的地图点和当前关键帧进行关联绑定
    const std::vector<MapPoint*> vpMapPointMatches = mpCurrentKeyFrame->GetMapPointMatches();
    // 对当前处理的这个关键帧中的所有的地图点展开遍历
    for (size_t i = 0; i < vpMapPointMatches.size(); i++) {
        MapPoint* pMP = vpMapPointMatches[i];
        if (pMP) {
            if (!pMP->isBad()) {
                if (!pMP->IsInKeyFrame(mpCurrentKeyFrame)) {
                    // 如果地图点不是来自当前帧的观测（比如来自局部地图点），为当前地图点添加观测
                    pMP->AddObservation(mpCurrentKeyFrame, i);
                    // 获得该点的平均观测方向和观测距离范围
                    pMP->UpdateNormalAndDepth();
                    // 更新地图点的最佳描述子
                    pMP->ComputeDistinctiveDescriptors();
                } else {    // this can only happen for new stereo points inserted by the Tracking
                    // 如果当前帧中已经包含了这个地图点，但是这个地图点却没有包含这个关键帧的信息
                    // 这些地图点可能来自双目或RGBD跟踪过程中新生成的地图点，或者是CreateNewMapPoints中通过三角化产生
                    // 将上述地图点放入mlpRecentAddedMapPoints，等待后续MapPointCulling的函数的检验
                    mlpRecentAddedMapPoints.push_back(pMP);
                }
            }
        }
    }

    // Step4 更新关键帧间的连接关系（共视图）
    mpCurrentKeyFrame->UpdateConnections();

    // Step5 将该关键帧插入到地图中
    mpMap->AddKeyFrame(mpCurrentKeyFrame);
}


/**
 * @brief 检查新增地图点，根据地图点的观测情况剔除质量不好的新增的地图点
 * mlpRecentAddedMapPoints:存储新增的地图点，这里是要删除其中不靠谱的
 * 
 * 步骤：
 * 1. 获得需要检查的新增地图点，根据相机类型设置不同的观测阈值，然后循环检查每个地图点
 * 2. 已经是坏点的地图点仅从队列中删除
 * 3. 跟踪到地图点的帧数相比预计可观测到该地图点的帧数的比例小于25%，从地图中删除
 *      3.1 mnVisible:在局部地图追踪函数TrackLocalMap()中调用了SearchLocalPoints(),
 *          其会判断该地图点是否在当前帧的视野范围内，如果在，说明其理应被当前帧(含普通帧)观测到，则会调用IncreaseVisible()对mnVisible执行+1操作。
 *      3.2 mvFound:在局部地图追踪函数TrackLocalMap()中，通过SearchLocalPoints()函数获得了
 *          更多的匹配关系之后，当前帧的每个地图点都会调用IncreaseFound()函数，对mnFound执行+1操作。
 * 4. 从该点建立开始，到现在已经过了不小于2个关键帧但观测到该点的相机数却不超过阈值cnThObs，设置为坏点，从地图中删除
 * 5. 从建立该点开始，已经过了3个关键帧而没有被剔除，则认为是质量高的点因此没有SetBadFlag()，仅从队列中删除
 */
void LocalMapping::MapPointCulling()
{
    // Check Recent Added MapPoints
    std::list<MapPoint*>::iterator lit = mlpRecentAddedMapPoints.begin();
    const unsigned long int nCurrentKFid = mpCurrentKeyFrame->mnId;

    // Step1 根据相机类型设置不同的观测阈值
    int nThObs;
    if (mbMonocular) {
        nThObs = 2;
    } else {
        nThObs = 3;
    }

    const int cnThObs = nThObs;

    // Step2 遍历检查新添加的地图点
    while (lit != mlpRecentAddedMapPoints.end()) {
        MapPoint* pMP = *lit;
        if (pMP->isBad()) {
            // Step2.1 已经是坏点的地图点仅从队列中删除
            lit = mlpRecentAddedMapPoints.erase(lit);
        } else if (pMP->GetFoundRatio() < 0.25f) {
            // Step2.2 跟踪到该地图点的帧数相比预计可观测到该地图点的帧数的比例小于25%，从地图中删除
            // （mnFound / mnVisible) < 25%
            // mnFound:地图点被多少帧（包括普通帧）看到，次数越多越好
            // mnVisible:地图点应该被看到的次数
            // (mnFound / mnVisible):对于大FOV镜头这个比例会高，对于窄FOV镜头这个比例会低
            pMP->SetBadFlag();
            lit = mlpRecentAddedMapPoints.erase(lit);
        } else if (((int)nCurrentKFid - (int)pMP->mnFirstKFid) >= 2 && pMP->Observations() <= cnThObs) {
            // Step2.3 从该点建立开始，到现在已经过了不小于2个关键帧
            // 但是观测到该点的相机数却不超过阈值cnThObs，从地图中删除
            pMP->SetBadFlag();
            lit = mlpRecentAddedMapPoints.erase(lit);
        } else if (((int)nCurrentKFid - (int)pMP->mnFirstKFid) >= 3) {
            // Step2.4 从建立该点开始，已经过了3个关键帧而没有被剔除，则认为是质量高的点
            // 因此没有SetBadFlag(),仅从队列中删除
            lit = mlpRecentAddedMapPoints.erase(lit);
        } else {
            lit++;
        }
    }
}


/**
 * @brief 用当前关键帧与相邻关键帧通过三角化产生新的地图点，使得跟踪更稳
 */
void LocalMapping::CreateNewMapPoints()
{
    // 不同传感器要求不一样,单目的时候需要有更多的具有较好共视关系的关键帧来建立地图
    // nn表示搜索最佳共视关键帧的数目
    int nn = 10;
    if (mbMonocular) {
        nn = 20;
    }

    // Step1 在当前关键帧的共视关键帧中找到共视程度最高的nn帧相邻关键帧
    const std::vector<KeyFrame*> vpNeighKFs = mpCurrentKeyFrame->GetBestCovisibilityKeyFrames(nn);

    // 特征点匹配配置,最佳距离<0.6*次佳距离,比较苛刻,不检查旋转.
    ORBmatcher matcher(0.6, false);

    // 取出当前帧从世界坐标系到相机坐标系的变换矩阵
    cv::Mat Rcw1 = mpCurrentKeyFrame->GetRotation();
    cv::Mat Rwc1 = Rcw1.t();
    cv::Mat tcw1 = mpCurrentKeyFrame->GetTranslation();
    cv::Mat Tcw1(3, 4, CV_32F);
    Rcw1.copyTo(Tcw1.colRange(0, 3));
    tcw1.copyTo(Tcw1.col(3));

    // 得到当前关键帧(左目)光心在世界坐标系中的坐标\内参
    cv::Mat Ow1 = mpCurrentKeyFrame->GetCameraCenter();

    const float &fx1 = mpCurrentKeyFrame->fx;
    const float &fy1 = mpCurrentKeyFrame->fy;
    const float &cx1 = mpCurrentKeyFrame->cx;
    const float &cy1 = mpCurrentKeyFrame->cy;
    const float &invfx1 = mpCurrentKeyFrame->invfx;
    const float &invfy1 = mpCurrentKeyFrame->invfy;

    // 用于后面的点深度的验证:这里的1.5是经验值
    // mfScaleFactor = 1.2
    const float ratioFactor = 1.5f * mpCurrentKeyFrame->mfScaleFactor;
    // 记录三角化成功的地图点数目
    int nnew = 0;

    // Step2 遍历相邻关键帧,搜索匹配并用极线约束剔除误匹配,最终三角化
    for (size_t i = 0; i < vpNeighKFs.size(); i++) {
        if (i > 0 && CheckNewKeyFrames()) {
            return;
        }

        KeyFrame* pKF2 = vpNeighKFs[i];

        // 相邻的关键帧光心在世界坐标系中的坐标
        cv::Mat Ow2 = pKF2->GetCameraCenter();
        // 基线向量,两个关键帧间的相机位移
        cv::Mat vBaseline = Ow2 - Ow1;
        // 基线长度
        const float baseline = cv::norm(vBaseline);

        // Step3 判断相机运动的基线是不是足够长
        if (!mbMonocular) {
            // 如果是双目相机,关键帧间距小于本身的基线时不生成3D点
            // 因为太短的基线下能够恢复的地图点不稳定
            if (baseline < pKF2->mb) {
                continue;
            }
        } else {
            // 单目相机情况
            // 相邻关键帧的场景深度中值
            const float medianDepthKF2 = pKF2->ComputeSceneMedianDepth(2);
            // 基线与景深的比例
            const float ratioBaselineDepth = baseline / medianDepthKF2;
            // 如果比例特别小,基线太短恢复3D点不准,那么跳过当前邻接的关键帧,不生成3D点
            if (ratioBaselineDepth < 0.01) {
                continue;
            }
        }

        // Step4 根据两个关键帧的位姿计算它们之间的基础矩阵
        cv::Mat F12 = ComputeF12(mpCurrentKeyFrame, pKF2);

        // Step5 通过词袋对两关键帧的未匹配的特征点快速匹配,用极线约束抑制离群点,生成新的匹配点对
        std::vector<std::pair<size_t, size_t>> vMatchedIndices;
        matcher.SearchForTriangulation(mpCurrentKeyFrame, pKF2, F12, vMatchedIndices, false);   // 最后一个值是bOnlyStereo

        // pKF2从世界坐标系到pKF2的位姿变换
        cv::Mat Rcw2 = pKF2->GetRotation();
        cv::Mat Rwc2 = Rcw2.t();
        cv::Mat tcw2 = pKF2->GetTranslation();
        cv::Mat Tcw2(3, 4, CV_32F);
        Rcw2.copyTo(Tcw2.colRange(0, 3));
        tcw2.copyTo(Tcw2.col(3));

        const float &fx2 = pKF2->fx;
        const float &fy2 = pKF2->fy;
        const float &cx2 = pKF2->cx;
        const float &cy2 = pKF2->cy;
        const float &invfx2 = pKF2->invfx;
        const float &invfy2 = pKF2->invfy;

        // Step6 对每对匹配点通过三角化生成3D点,和Triangulate函数差不多
        const int nmatches = vMatchedIndices.size();
        for (int ikp = 0; ikp < nmatches; ikp++) {
            // Step6.1 取出匹配特征点
            // 当前匹配对在当前关键帧中的索引
            const int &idx1 = vMatchedIndices[ikp].first;
            // 当前匹配对在邻接关键帧中的索引
            const int &idx2 = vMatchedIndices[ikp].second;

            // 当前匹配在当前关键帧中的特征点
            const cv::KeyPoint &kp1 = mpCurrentKeyFrame->mvKeysUn[idx1];
            // mvuRight中存放着双目的深度值,如果不是双目,其值将为-1
            const float kp1_ur = mpCurrentKeyFrame->mvuRight[idx1];
            bool bStereo1 = kp1_ur >= 0;

            // 当前匹配在邻接关键帧中的特征点
            const cv::KeyPoint &kp2 = pKF2->mvKeysUn[idx2];
            // mvuRight中存放着双目的深度值,如果不是双目,其值为-1
            const float kp2_ur = pKF2->mvuRight[idx2];
            bool bStereo2 = kp2_ur >= 0;

            // Step6.2 利用匹配点反投影得到的视差角
            // 特征点反投影,其实得到的是在各自相机坐标系下的一个归一化的方向向量,和这个点的反投影射线重合
            // 这是将两个关键点的相机图像坐标系下的2D坐标投影到相机归一化坐标系下
            // z*u = x*fx + z*cx, z*v = y*fy + z*cy
            // ==> x = z*(u - cx) / fx, y = z*(v - cy) / fy
            cv::Mat xn1 = (cv::Mat_<float>(3, 1) << (kp1.pt.x - cx1) * invfx1, (kp1.pt.y - cy1) * invfy1, 1.0);
            cv::Mat xn2 = (cv::Mat_<float>(3, 1) << (kp2.pt.x - cx2) * invfx2, (kp2.pt.y - cy2) * invfy2, 1.0);

            // 由相机坐标系转到世界坐标系(得到的是那条反投影射线的一个同向向量在世界坐标系下的表示,还是只能够表示方向),得到视差角余弦
            cv::Mat ray1 = Rwc1 * xn1;
            cv::Mat ray2 = Rwc2 * xn2;
            // 匹配点射线夹角余弦值
            const float cosParallaxRays = ray1.dot(ray2) / (cv::norm(ray1) * cv::norm(ray2));

            // 加1是为了让cosParallaxStereo随便初始化为一个很大的值
            float cosParallaxStereo = cosParallaxRays + 1;
            // cosParallaxStereo1,cosParallaxStereo2在后面可能不存在,需要初始化为较大的值
            float cosParallaxStereo1 = cosParallaxStereo;
            float cosParallaxStereo2 = cosParallaxStereo;

            // Step6.3 对于双目,利用双目得到视差角:单目相机没有特殊操作
            // θ = arctan(0.5*b/depth),cos(2θ)=上面的两条射线的夹角
            // 传感器是双目相机,并且当前的关键帧的这个点有对应的深度
            // 假设是平行的双目相机,计算出双目相机观察这个点的时候的视差角余弦
            // 是左右两个相机光心与目标地图点形成的夹角,baseline与深度形成90°夹角,所以夹角的一半=arctan(baseline/2, depth)
            if (bStereo1) {
                cosParallaxStereo1 = std::cos(2 * std::atan2(mpCurrentKeyFrame->mb / 2, mpCurrentKeyFrame->mvDepth[idx1]));
            } else if (bStereo2) {
                // 传感器是双目相机,并且相邻的关键帧的这个点有对应的深度,和上面一样的操作
                cosParallaxStereo2 = std::cos(2 * std::atan2(pKF2->mb / 2, pKF2->mvDepth[idx2]));
            }

            // 得到双目观测的视差角中最小的那个
            cosParallaxStereo = std::min(cosParallaxStereo1, cosParallaxStereo2);   // cos值越小,夹角越大.这里是要获得当前帧和pKF2两个帧视差角较大的那个帧

            // Step6.4 三角化恢复3D点
            cv::Mat x3D;
            // 匹配点对夹角大,用三角法恢复3D点
            // cosParallaxRays是地图点在两个帧(左目)的射线夹角
            // cosParallaxRays < cosParallaxStereo 表示地图点在当前帧或pKF2中的夹角大于在两个帧之间的夹角,表明匹配点对夹角大于双目本身观察三维点夹角
            // cosParallaxRays > 0 表示角度范围在0~90°之间
            // cosParallaxRays < 0.9998 表示角度大于1°,0.9998对应1°
            if (cosParallaxRays < cosParallaxStereo && cosParallaxRays > 0 && (bStereo1 || bStereo2 || cosParallaxRays < 0.9998)) {
                // Linear Triangulation Method
                // 见Initializer.cc的Triangulate函数，实现是一样的，顶多就是把投影矩阵换成了变换矩阵
                cv::Mat A(4, 4, CV_32F);
                A.row(0) = xn1.at<float>(0) * Tcw1.row(2) - Tcw1.row(0);    // x1 * ()
                A.row(1) = xn1.at<float>(1) * Tcw1.row(2) - Tcw1.row(1);    //
                A.row(2) = xn2.at<float>(0) * Tcw2.row(2) - Tcw2.row(0);    //
                A.row(3) = xn2.at<float>(1) * Tcw2.row(2) - Tcw2.row(1);    //

                cv::Mat w, u, vt;
                cv::SVD::compute(A, w, u, vt, cv::SVD::MODIFY_A | cv::SVD::FULL_UV);

                x3D = vt.row(3).t();
                // 归一化之前的检查
                if (x3D.at<float>(3) == 0) {
                    continue;
                }

                // 归一化称为齐次坐标,然后提取前面三个维度作为欧式坐标
                x3D = x3D.rowRange(0, 3) / x3D.at<float>(3);
            } else if (bStereo1 && cosParallaxStereo1 < cosParallaxStereo2) {
                // 匹配点对夹角小,用双目恢复3D点
                // 如果是双目,用视差角更大的那个双目信息来恢复,直接用已知3D点反投影了
                x3D = mpCurrentKeyFrame->UnprojectStereo(idx1);
            } else if (bStereo2 && cosParallaxStereo2 < cosParallaxStereo1) {
                x3D = pKF2->UnprojectStereo(idx2);
            } else {
                continue;   // No stereo and very low paralax
            }

            cv::Mat x3Dt = x3D.t();

            // Step6.5 检测生成的3D点是否在相机前方,不在的话就放弃这个点
            float z1 = Rcw1.row(2).dot(x3Dt) + tcw1.at<float>(2);
            if (z1 <= 0) {
                continue;
            }

            float z2 = Rcw2.row(2).dot(x3Dt) + tcw2.at<float>(2);
            if (z2 <= 0) {
                continue;
            }

            // Step6.6 计算3D点在当前关键帧下的重投影误差
            const float &sigmaSquare1 = mpCurrentKeyFrame->mvLevelSigma2[kp1.octave];
            const float x1 = Rcw1.row(0).dot(x3Dt) + tcw1.at<float>(0);
            const float y1 = Rcw1.row(1).dot(x3Dt) + tcw1.at<float>(1);
            const float invz1 = 1.0 / z1;

            if (!bStereo1) {
                // 单目情况下
                float u1 = fx1 * x1 * invz1 + cx1;
                float v1 = fy1 * y1 * invz1 + cy1;
                float errX1 = u1 - kp1.pt.x;
                float errY1 = v1 - kp1.pt.y;
                // 假设测量有一个像素的偏差，2自由度卡方检验阈值是5.991
                if ((errX1*errX1 + errY1*errY1) > 5.991 * sigmaSquare1) {
                    continue;
                }
            } else {
                // 非单目情况，根据视差公式计算假想的右目坐标
                float u1 = fx1 * x1 * invz1 + cx1;
                float u1_r = u1 - mpCurrentKeyFrame->mbf * invz1;
                float v1 = fy1 * y1 * invz1 + cy1;
                float errX1 = u1 - kp1.pt.x;
                float errY1 = v1 - kp1.pt.y;
                float errX1_r = u1_r - kp1_ur;
                // 自由度为3，卡方检验阈值是7.8
                if ((errX1*errX1 + errY1*errY1 + errX1_r*errX1_r) > 7.8 * sigmaSquare1) {
                    continue;
                }
            }

            // 计算3D点在另一个关键帧下的重投影误差，操作同上
            const float sigmaSquare2 = pKF2->mvLevelSigma2[kp2.octave];
            const float x2 = Rcw2.row(0).dot(x3Dt) + tcw2.at<float>(0);
            const float y2 = Rcw2.row(1).dot(x3Dt) + tcw2.at<float>(1);
            const float invz2 = 1.0 / z2;
            if (!bStereo2) {
                float u2 = fx2 * x2 * invz2 + cx2;
                float v2 = fy2 * y2 * invz2 + cy2;
                float errX2 = u2 - kp2.pt.x;
                float errY2 = v2 - kp2.pt.y;
                if ((errX2*errX2 + errY2*errY2) > 5.991*sigmaSquare2) {
                    continue;
                }
            } else {
                float u2 = fx2 * x2 * invz2 + cx2;
                float u2_r = u2 - mpCurrentKeyFrame->mbf * invz2;
                float v2 = fy2 * y2 * invz2 + cy2;
                float errX2 = u2 - kp2.pt.x;
                float errY2 = v2 - kp2.pt.y;
                float errX2_r = u2_r - kp2_ur;
                if ((errX2*errX2 + errY2*errY2 + errX2_r*errX2_r) > 7.8 * sigmaSquare2) {
                    continue;
                }
            }

            // Step6.7 检查尺度连续性
            // 世界坐标系下，3D点与相机间的向量，方向由相机指向3D点
            cv::Mat normal1 = x3D - Ow1;
            float dist1 = cv::norm(normal1);
            
            cv::Mat normal2 = x3D - Ow2;
            float dist2 = cv::norm(normal2);

            if (dist1 == 0 || dist2 == 0) {
                continue;
            }

            // ratioDist 是不考虑金字塔尺度下的距离比例
            const float ratioDist = dist2 / dist1;
            // 金字塔尺度因子的比例
            const float ratioOctave = mpCurrentKeyFrame->mvScaleFactors[kp1.octave] / pKF2->mvScaleFactors[kp2.octave];

            // 距离的比例和图像金字塔的比例不应该差太多，否则就跳过
            if (ratioDist*ratioFactor < ratioOctave || ratioDist > ratioOctave*ratioFactor) {
                continue;
            }

            // Step6.8 三角化生成3D点成功，构造MapPoint
            MapPoint* pMP = new MapPoint(x3D, mpCurrentKeyFrame, mpMap);

            // Step6.9 为该MapPoint添加属性
            // a. 观测到该MapPoint的关键帧
            pMP->AddObservation(mpCurrentKeyFrame, idx1);
            pMP->AddObservation(pKF2, idx2);

            mpCurrentKeyFrame->AddMapPoint(pMP, idx1);
            pKF2->AddMapPoint(pMP, idx2);

            // b. 该MapPoint的描述子
            pMP->ComputeDistinctiveDescriptors();

            // c. 该MapPoint的平均观测方向和深度范围
            pMP->UpdateNormalAndDepth();

            mpMap->AddMapPoint(pMP);

            // Step6.10 将新产生的点放入检测队列
            // 这些MapPoints都会经过MapPointCulling函数的检验
            mlpRecentAddedMapPoints.push_back(pMP);

            nnew++;            
        }
    }
}


/**
 * @brief 检查并融合当前关键帧与相邻帧（两级相邻）重复的地图点
 * 
 */
void LocalMapping::SearchInNeighbors()
{
    // Step1 获得当前关键帧在共视图中权重排名前nn的邻接关键帧
    // 开始之前先定义几个概念
    // 当前关键帧的相邻关键帧，称为一级相邻关键帧，也就是邻居
    // 与一级相邻关键帧相邻的关键帧，称为二级相邻关键帧，也就是邻居的邻居

    // 单目情况要20个邻接关键帧，双目或RGBD则要10个
    int nn = 10;
    if (mbMonocular) {
        nn = 20;
    }

    // 和当前关键帧相邻的关键帧，也就是一级相邻关键帧
    const std::vector<KeyFrame*> vpNeighKFs = mpCurrentKeyFrame->GetBestCovisibilityKeyFrames(nn);

    // Step2 存储一级相邻关键帧及二级相邻关键帧
    std::vector<KeyFrame*> vpTargetKFs;
    // 开始对所有候选的一级关键帧展开遍历
    for (std::vector<KeyFrame*>::const_iterator vit = vpNeighKFs.begin(), vend = vpNeighKFs.end(); vit != vend; vit++) {
        KeyFrame* pKFi = *vit;
        // 没有和当前帧进行过融合的操作
        if (pKFi->isBad() || pKFi->mnFuseTargetForKF == mpCurrentKeyFrame->mnId) {
            continue;
        }

        // 加入一级相邻关键帧
        vpTargetKFs.push_back(pKFi);
        // 标记已经加入
        pKFi->mnFuseTargetForKF = mpCurrentKeyFrame->mnId;

        // 以一级相邻关键帧的共视关系最好的5个相邻关键帧作为二级相邻关键帧
        const std::vector<KeyFrame*> vpSecondNeighKFs = pKFi->GetBestCovisibilityKeyFrames(5);
        // 遍历得到的二级相邻关键帧
        for (std::vector<KeyFrame*>::const_iterator vit2 = vpSecondNeighKFs.begin(), vend2 = vpSecondNeighKFs.end(); vit2 != vend2; vit2++) {
            KeyFrame* pKFi2 = *vit2;
            // 当然这个二级相邻关键帧要求没有和当前关键帧发生融合，并且这个二级相邻关键帧也不是当前关键帧
            if (pKFi2->isBad() || pKFi2->mnFuseTargetForKF == mpCurrentKeyFrame->mnId || pKFi2->mnId == mpCurrentKeyFrame->mnId) {
                continue;
            }
            // 存入二级相邻关键帧
            vpTargetKFs.push_back(pKFi2);
        }
    }

    // 使用默认参数，最优和次优比例0.6，匹配时检查特征点的旋转
    ORBmatcher matcher;

    // Step3 将当前帧的地图点分别投影到一级和二级相邻关键帧，寻找匹配点对应的地图点进行融合，称为正向投影融合
    std::vector<MapPoint*> vpMapPointMatches = mpCurrentKeyFrame->GetMapPointMatches();
    for (std::vector<KeyFrame*>::iterator vit = vpTargetKFs.begin(), vend = vpTargetKFs.end(); vit != vend; vit++) {
        KeyFrame* pKFi = *vit;

        // 将地图点投影到关键帧中进行匹配和融合：融合策略如下
        // 1. 如果地图点能匹配关键帧的特征点，并且该点有对应的地图点，那么选择观测数目多的替换两个地图点
        // 2. 如果地图点能匹配关键帧的特征点，并且该点没有对应的地图点，那么为该点添加该投影地图点
        // 注意这个时候对地图点融合的操作是立即生效的
        matcher.Fuse(pKFi, vpMapPointMatches);
    }

    // Step4 将两级相邻关键帧地图点分别投影到当前关键帧，寻找匹配点对应的地图点进行融合，称为反向投影融合
    // 用于进行存储要融合的一级邻接和二级邻接关键帧所有MapPoints的集合
    std::vector<MapPoint*> vpFuseCandidates;
    vpFuseCandidates.reserve(vpTargetKFs.size() * vpMapPointMatches.size());    // 关键帧数量*当前帧地图点数量，最大的内存空间是所有当前帧地图点都可以投影到每个关键帧

    // Step4.1 遍历每一个一级邻接和二级邻接关键帧，收集他们的地图点存储到 vpFuseCandidates
    for (std::vector<KeyFrame*>::iterator vitKF = vpTargetKFs.begin(), vendKF = vpTargetKFs.end(); vitKF != vendKF; vitKF++) {
        KeyFrame* pKFi = *vitKF;
        std::vector<MapPoint*> vpMapPointsKFi = pKFi->GetMapPointMatches();

        // 遍历当前一级邻接和二级邻接关键帧中所有的MapPoints，找出需要进行融合的并且加入到集合中
        for (std::vector<MapPoint*>::iterator vitMP = vpMapPointsKFi.begin(), vendMP = vpMapPointsKFi.end(); vitMP != vendMP; vitMP++) {
            MapPoint* pMP = *vitMP;
            if (!pMP) {
                continue;
            }
            
            // 如果地图点是坏点，或者已经加进集合vpFuseCandidates，跳过
            if (pMP->isBad() || pMP->mnFuseCandidateForKF == mpCurrentKeyFrame->mnId) {
                continue;
            }

            // 加入集合，并标记已经加入
            pMP->mnFuseCandidateForKF = mpCurrentKeyFrame->mnId;
            vpFuseCandidates.push_back(pMP);
        }
    }

    // Step4.2 进行地图点投影融合，和正向融合操作时完全相同
    // 不同的是正向操作是“每个关键帧和当前关键帧的地图点进行融合”，而这里的是“当前关键帧和所有邻接关键帧的地图点进行融合”
    matcher.Fuse(mpCurrentKeyFrame, vpFuseCandidates);

    // Step5 更新当前关键帧地图点的描述子，深度，平均观测方向等属性
    vpMapPointMatches = mpCurrentKeyFrame->GetMapPointMatches();
    for (size_t i = 0, iend = vpMapPointMatches.size(); i < iend; i++) {
        MapPoint* pMP = vpMapPointMatches[i];
        if (pMP) {
            if (!pMP->isBad()) {
                // 在所有找到pMP的关键帧中，获得最佳的描述子
                pMP->ComputeDistinctiveDescriptors();
                // 更新平均观测方向和观测距离
                pMP->UpdateNormalAndDepth();
            }
        }
    }

    // Step6 更新当前帧与其它帧的共视连接关系
    mpCurrentKeyFrame->UpdateConnections();
}


/**
 * @brief 检测当前关键帧在共视图中的关键帧，根据地图点在共视图中的冗余程度剔除该共视关键帧
 * 冗余关键帧的判定：90%以上的地图点能被其它关键帧(至少3个)观测到
 * 
 * 
 * 步骤：
 * 1. 根据共视图提取当前关键帧mpCurrentKeyFrame的所有共视关键帧，存储于变量vpLocalKeyFrames之中
 * 2. 对所有的共视关键帧进行遍历，先判断共视关键帧是否为第一帧关键帧，如果为第一帧关键帧则跳过（第一帧关键帧不会被删除），然后提取
 *      共视关键帧的所有地图点存放于vpMapPoints之中
 * 3. 对共视关键帧的所有地图点vpMapPoints进行遍历，如果地图点pMP是坏的则跳过。然后统计地图点能够被多少关键帧观测到，如果超过thObs=3，则nRedundantObservations进行+1操作
 * 4. 遍历完共视关键帧的所有地图之后会跳出循环，然后判断nRedundantObservations的数目是否超过该共视关键帧地图数的90%，如果超过了则通过
 *      pKF->SetBadFlag()告知系统该共视关键帧需要删除。这里需要注意，其并没有马上删除该关键帧-->这个关键帧有可能正在回环检测或者计算Sim3操作，这时候虽然这个关键帧冗余，
 *      但是却不能删除，仅设置mbNotErase为true，这时候调用setBadFlag函数时，不会将这个关键帧删除，只会把mbTobeErase变为true，代表这个关键帧可以删除但不到时候，先记下来以后处理。
 *      在闭环线程里调用SetErase()会根据mbToBeErased来删除之前可以删除但还没删除的帧。
 */
void LocalMapping::KeyFrameCulling()
{
    // 该函数里变量层层深入，这里列一下：
    // mpCurrentKeyFrame:   当前关键帧，本程序就是判断它是否需要删除
    // pKF:                 mpCurrentKeyFrame的某一个共视关键帧
    // vpMapPoints:         pKF对应的所有地图点
    // pMP:                 vpMapPoints中的某个地图点
    // observations:        所有能观测到pMP的关键帧
    // pKFi:                observations中的某个关键帧
    // scaleLeveli:         pKFi的金字塔尺度
    // scaleLevel:          pKF的金字塔尺度

    // Step1 根据共视图提取当前关键帧的所有共视关键帧
    std::vector<KeyFrame*> vpLocalKeyFrames = mpCurrentKeyFrame->GetVectorCovisibleKeyFrames();

    // 对所有的共视关键帧进行遍历
    for (std::vector<KeyFrame*>::iterator vit = vpLocalKeyFrames.begin(), vend = vpLocalKeyFrames.end(); vit != vend; vit++) {
        KeyFrame* pKF = *vit;
        // 第1个关键帧不能删除，跳过
        if (pKF->mnId == 0) {
            continue;
        }

        // Step2 提取每个共视关键帧的地图点
        const std::vector<MapPoint*> vpMapPoints = pKF->GetMapPointMatches();

        // 记录某个点被观测次数，后面并未使用
        int nObs = 3;
        // 观测次数阈值，默认为3
        const int thObs = nObs;
        // 记录冗余观测点的数目
        int nRedundantObservations = 0;
        int nMPs = 0;

        // Step3 遍历该共视关键帧的所有地图点，其中能被其它至少3个关键帧观测到的地图点为冗余地图点
        for (size_t i = 0, iend = vpMapPoints.size(); i < iend; i++) {
            MapPoint* pMP = vpMapPoints[i];
            if (pMP) {
                if (!pMP->isBad()) {
                    if (!mbMonocular) {
                        // 对于双目或RGBD，仅考虑近处（不超过基线的40倍）的地图点
                        if (pKF->mvDepth[i] > pKF->mThDepth || pKF->mvDepth[i] < 0) {
                            continue;
                        }
                    }
                    nMPs++;
                    // Observations()是观测到该地图点的相机总数目
                    if (pMP->Observations() > thObs) {  // thObs = 3
                        const int &scaleLevel = pKF->mvKeysUn[i].octave;
                        // Observation 存储的是可以看到该地图点的所有关键帧的集合
                        const std::map<KeyFrame*, size_t> observations = pMP->GetObservations();
                        int nObs = 0;
                        // 遍历观测到该地图点的关键帧，统计该地图点观察帧
                        for (std::map<KeyFrame*, size_t>::const_iterator mit = observations.begin(), mend = observations.end(); mit != mend; mit++) {
                            KeyFrame* pKFi = mit->first;
                            if (pKFi == pKF) {
                                continue;
                            }
                            const int &scaleLeveli = pKFi->mvKeysUn[mit->second].octave;

                            // 尺度约束：为什么pKF，尺度+1 要大于等于 pKFi的尺度？
                            // 回答：因为同样或更低金字塔层级的地图点更准确
                            if (scaleLeveli <= scaleLevel + 1) {
                                nObs++;
                                // 已经找到3个满足条件的关键帧，就停止不找了
                                if (nObs >= thObs) {
                                    break;
                                }
                            }
                        }
                        // 地图点至少被3个满足条件的关键帧，就停止不找了
                        if (nObs >= thObs) {
                            nRedundantObservations++;
                        }
                    }
                }
            }
        }

        // Step4 如果该关键帧90%以上的有效地图点被判断为冗余的，则认为该关键帧是冗余的，需要删除该关键帧
        if (nRedundantObservations > 0.9 * nMPs) {
            pKF->SetBadFlag();
        }
    }
}


bool LocalMapping::Stop()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    if (mbStopRequested && !mbNotStop) {
        mbStopped = true;
        std::cout << "Local Mapping STOP" << std::endl;
        return true;
    }
    return false;
}


bool LocalMapping::CheckFinish()
{
    std::unique_lock<std::mutex> lock(mMutexFinish);
    return mbFinishRequested;
}


void LocalMapping::SetFinish()
{
    std::unique_lock<std::mutex> lock(mMutexFinish);
    mbFinished = true;
    std::unique_lock<std::mutex> lock2(mMutexStop);
    mbStopped = true;
}


void LocalMapping::ResetIfRequested()
{
    std::unique_lock<std::mutex> lock(mMutexReset);
    if (mbResetRequested) {
        mlNewKeyFrames.clear();
        mlpRecentAddedMapPoints.clear();
        mbResetRequested = false;
    }
}


cv::Mat LocalMapping::ComputeF12(KeyFrame* &pKF1, KeyFrame* &pKF2)
{
    cv::Mat R1w = pKF1->GetRotation();
    cv::Mat t1w = pKF1->GetTranslation();
    cv::Mat R2w = pKF2->GetRotation();
    cv::Mat t2w = pKF2->GetTranslation();

    cv::Mat R12 = R1w * R2w.t();
    cv::Mat t12 = -R1w * R2w.t() * t2w + t1w;

    cv::Mat t12x = SkewSymmetricMatrix(t12);

    const cv::Mat &K1 = pKF1->mK;
    const cv::Mat &K2 = pKF2->mK;

    return K1.t().inv() * t12x * R12 * K2.inv();
}


cv::Mat LocalMapping::SkewSymmetricMatrix(const cv::Mat &v)
{
    return (cv::Mat_<float>(3, 3) << 0, -v.at<float>(2), v.at<float>(1),
                                    v.at<float>(2), 0, -v.at<float>(0),
                                    -v.at<float>(1), v.at<float>(0), 0);
}

}