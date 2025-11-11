#include "System.h"

namespace ORB_SLAM2
{

System::System(const std::string &strVocFile, const std::string &strSettingsFile, const eSensor sensor, const bool bUseViewer)
    : mSensor(sensor), mbReset(false), mbActivateLocalizationMode(false), mbDeactivateLocalizationMode(false)
{
    std::cout << "ORB-SLAM2 Copyright (C) 2014-2016 Raul Mur-Artal, University of Zaragoza." << std::endl <<
    "This program comes with ABSOLUTELY NO WARRANTY;" << std::endl <<
    "This is free software, and you are welcome to redistribute it" << std::endl <<
    "under certain conditions. See LICENSE.txt." << std::endl << std::endl;

    // 输出当前传感器类型
    std::cout << "Input sensor was set to: ";

    if (mSensor == MONOCULAR) {
        std::cout << "Monocular" << std::endl;
    } else if (mSensor == STEREO) { 
        std::cout << "Stereo" << std::endl;
    } else if (mSensor == RGBD) {
        std::cout << "RGB-D" << std::endl;
    }

    // Step1 初始化各成员变量
    // Step1.1 读取配置文件信息
    cv::FileStorage fsSettings(strSettingsFile.c_str(), cv::FileStorage::READ);
    if (!fsSettings.isOpened()) {
        std::cerr << "Failed to open settings file at: " << strSettingsFile << std::endl;
        exit(-1);
    }

    // Load ORB Vocabulary
    std::cout << std::endl << "Loading ORB Vocabulary. This could take a while..." << std::endl;

    // Step1.2 创建ORB词袋，字典树的主要作用是：特征匹配，关键帧辨别
    mpVocabulary = new ORBVocabulary();
    bool bVocLoad = mpVocabulary->loadFromTextFile(strVocFile);
    if (!bVocLoad) {
        std::cerr << "Wrong path to vocabulary. " << std::endl;
        std::cerr << "Failed to open at: " << strVocFile << std::endl;
        exit(-1);
    }
    std::cout << "Vocabulary loaded!" << std::endl << std::endl;

    // Step1.3 创建关键帧数据库，主要保存ORB描述子倒排索引(即根据描述子查找拥有该描述子的关键帧)
    mpKeyFrameDatabase = new KeyFrameDatabase(*mpVocabulary);

    // Step1.4 创建地图
    mpMap = new Map();

    // Create Drawer. These are used by the Viewer
    // 这里的帧绘制器和地图绘制器都将会被可视化的Viewer所使用
    mpFrameDrawer = new FrameDrawer(mpMap);                 // 帧绘制器
    mpMapDrawer   = new MapDrawer(mpMap, strSettingsFile);  // 地图绘制器

    // Step2 创建三大线程：Tracking、LocalMapping、LoopClosing
    // Step2.1 主线程就是Tracking线程，只需要创建Tracking对象即可
    mpTracker = new Tracking(this,                  // Tracking类需要传入
                             mpVocabulary,          // 字典
                             mpFrameDrawer,         // 帧绘制器
                             mpMapDrawer,           // 地图绘制器
                             mpMap,                 // 地图
                             mpKeyFrameDatabase,    // 关键帧数据库
                             strSettingsFile,        // 设置文件路径
                             mSensor);              // 传感器类型
    
    // Step2.2 创建LocalMapping线程及mpLocalMapper
    mpLocalMapper = new LocalMapping(mpMap, mSensor == MONOCULAR);
    mptLocalMapping = new std::thread(&ORB_SLAM2::LocalMapping::Run, mpLocalMapper);

    // Step2.3 创建LoopClosing线程及mpLoopCloser
    mpLoopCloser = new LoopClosing(mpMap,                   // 地图
                                   mpKeyFrameDatabase,      // 关键帧数据库
                                   mpVocabulary,            // ORB字典
                                   mSensor != MONOCULAR);   // 当前的传感器是否是单目
    mptLoopClosing = new std::thread(&ORB_SLAM2::LoopClosing::Run, mpLoopCloser);

    if (bUseViewer) {
        mpViewer = new Viewer(this, mpFrameDrawer, mpMapDrawer, mpTracker, strSettingsFile);
        mptViewer = new std::thread(&Viewer::Run, mpViewer);
        mpTracker->SetViewer(mpViewer);
    }

    // Step3 设置线程间通信
    mpTracker->SetLocalMapper(mpLocalMapper);
    mpTracker->SetLoopClosing(mpLoopCloser);

    mpLocalMapper->SetTracker(mpTracker);
    mpLocalMapper->SetLoopCloser(mpLoopCloser);

    mpLoopCloser->SetTracker(mpTracker);
    mpLoopCloser->SetLocalMapper(mpLocalMapper);
}


cv::Mat System::TrackRGBD(const cv::Mat &im, const cv::Mat &depthmap, const double &timestamp)
{
    if (mSensor != RGBD)
    {
        std::cerr << "ERROR: you called TrackRGBD but input sensor was not set to RGBD." << std::endl;
        exit(-1);
    }

    // Check mode change
    {
        // 独占锁，为了mbActivateLocalizationMode和
        std::unique_lock<std::mutex> lock(mMutexMode);
        if (mbActivateLocalizationMode) {
            mpLocalMapper->RequestStop(); // mbStopRequested = true， mbAbortBA = true

            // Wait until Local Mapping has effectively stopped
            while (!mpLocalMapper->isStopped()) { // return mbStopped
                std::this_thread::sleep_for(std::chrono::microseconds(1000)); // 休眠1毫秒
            }

            // 局部地图关闭之后，跟踪线程只进行跟踪，只计算相机的位姿，没有对局部地图进行更新
            mpTracker->InformOnlyTracking(true); // mbOnlyTracking = flag;
            // 关闭线程可以让别的线程得到更多的资源
            mbActivateLocalizationMode = false;
            
        }

        // 如果 mbDeactivateLocalizationMode 是true，局部线程的关闭状态就会被释放，关键帧从局部地图中删除
        if (mbDeactivateLocalizationMode) {
            mpTracker->InformOnlyTracking(false); // 置为false，表示Tracking线程退出仅跟踪模式
            mpLocalMapper->Release();   // 将 mbStopped 置为 false，将关键帧全部清除，因为要重新开始LocalMapper线程
            mbDeactivateLocalizationMode = false;
        }
    }

    // Check reset
    {
        std::unique_lock<std::mutex> lock(mMutexReset);
        if (mbReset) {
            mpTracker->Reset();
            mbReset = false;
        }
    }

    // 获取相机位姿的估计结果
    cv::Mat Tcw = mpTracker->GrabImageRGBD(im, depthmap, timestamp);

    std::unique_lock<std::mutex> lock2(mMutexState);
    mTrackingState      = mpTracker->mState;                        // 跟踪状态
    mTrackedMapPoints   = mpTracker->mCurrentFrame.mvpMapPoints;    // 跟踪到的地图点
    mTrackedKeyPointsUn = mpTracker->mCurrentFrame.mvKeysUn;        // 跟踪到的关键点

    return Tcw;
}


void System::Shutdown()
{
    mpLocalMapper->RequestFinish();
    mpLoopCloser->RequestFinish();
    if (mpViewer) {
        mpViewer->RequestFinish();
        while (!mpViewer->isFinished()) {
            std::this_thread::sleep_for(std::chrono::microseconds(5000));
        }
    }
    
    // Wait until all thread have effectively stopped
    while (!mpLocalMapper->isFinished() || !mpLoopCloser->isFinished() || mpLoopCloser->isRunningGBA()) {
        std::this_thread::sleep_for(std::chrono::microseconds(5000));
    }

    if (mpViewer) {
        pangolin::BindToContext("ORB-SLAM2: Map Viewer");
    }
}


/**
 * @brief 保存关键帧到第一个关键帧之间的位姿
 * 旋转矩阵使用四元数来表示
 */
void System::SaveTrajectoryTUM(const std::string &filename)
{
    std::cout << std::endl << "Saving camera trajectory to " << filename << " ..." << std::endl;
    if (mSensor == MONOCULAR)
    {
        std::cerr << "ERROR: SaveTrajectoryTUM cannot be used for monocular." << std::endl;
        return;
    }

    std::vector<KeyFrame*> vpKFs = mpMap->GetAllKeyFrames();
    std::sort(vpKFs.begin(), vpKFs.end(), KeyFrame::lId);   // 根据KeyFrame的mnId来排序，从小到大排序，目的是为了获取最早的关键帧
                                                            // 题外话：排序的时间复杂度是O(nlogn)，如果选择直接遍历应该会更快一些？？？？？？？？？？？？？

    // Transform all keyframes so that the first keyframe is at the origin.
    // After a loop closure the first keyframe might not be at the origin.
    cv::Mat Two = vpKFs[0]->GetPoseInverse();   // 第一帧相机原点在世界坐标系下的位姿，作为原点帧

    std::ofstream f;
    f.open(filename.c_str());
    f << std::fixed;

    // Frame pose is stored relative to its reference keyframe (which is optimized by BA and pose graph). 帧位姿是相对于其参考关键帧(通过BA优化和位姿图优化)存储的。
    // We need to get first the keyframe pose and then concatenate the relative transformation. 获取时需要先提取关键帧位姿，再叠加相对变换。
    // Frames not localized (tracking failure) are not saved. 未完成定位（跟踪失败）的帧不会被保存

    // For each frame we have a reference keyframe (lRit), the timestamp (lT) and a flag
    // which is true when tracking failed (lbL).    对于每个帧，我们记录其参考关键帧索引（lRit）、时间戳（lT）和一个标记位（lbL），该标记为true时表示跟踪失败。
    std::list<KeyFrame*>::iterator lRit = mpTracker->mlpReferences.begin(); // 参考关键帧列表
    std::list<double>::iterator lT = mpTracker->mlFrameTimes.begin();       // 时间戳
    std::list<bool>::iterator lbL = mpTracker->mlbLost.begin();             // mlpReferences对应的帧的跟踪状态是否处于Lost状态
    // mlRelativeFramePoses保存的是参考帧到当前帧的相对位姿变换
    for (std::list<cv::Mat>::iterator lit = mpTracker->mlRelativeFramePoses.begin(), lend = mpTracker->mlRelativeFramePoses.end();
         lit != lend; lit++, lRit++, lT++, lbL++) {
        if (*lbL) { // 如果遍历的这个帧处于
            continue;
        }

        KeyFrame* pKF = *lRit;  // 遍历参考帧

        cv::Mat Trw = cv::Mat::eye(4, 4, CV_32F);   // 将每个参考帧作为原点帧，目的是：当前参考帧有可能被剔除，就需要获取到一帧合适的关键帧，记录的是父关键帧到参考帧的位姿变换

        // If the reference keyframe was culled, traverse the spanning tree to get a suitable keyframe.
        // 如果参考关键帧被剔除，则遍历 spanning tree（生成树）以获取一个合适的关键帧。
        while (pKF->isBad()) {
            Trw = Trw * pKF->mTcp;  // mTcp表示父关键帧到pKF当前帧的位姿变换，都是在世界坐标系中的位姿。这里是计算最终选择的pKF(可能是pKF的夫关键帧)到最初遍历的关键帧pKF的位姿
            pKF = pKF->GetParent(); // 如果pKF被剔除了，就寻找其父关键帧
        }

        Trw = Trw * pKF->GetPose() * Two;   // Two表示原点帧到世界坐标系中的位姿；pKF->GetPose表示选择的当前帧pKF在世界坐标系中的位姿，也就是Tcw；Trw表示pKF到参考帧的位姿，
                                            // 所以，最终Trw就表示从原点帧到参考帧pKF的位姿

        cv::Mat Tcw = (*lit) * Trw; // Trw是原点帧到参考帧pKF的位姿, *lit是参考帧到当前帧的位姿变换
                                    // 所以结果就是原点帧到参考帧的当前帧之间的位姿变换
        cv::Mat Rwc = Tcw.rowRange(0, 3).colRange(0, 3).t();
        cv::Mat twc = -Rwc*Tcw.rowRange(0, 3).col(3);       // 这里计算的是当前帧到原点帧的位移

        std::vector<float> q = Converter::toQuaternion(Rwc);

        f << std::setprecision(6) << *lT << " " << std::setprecision(9) << twc.at<float>(0) << " " << twc.at<float>(1) << " " << twc.at<float>(2)
          << " " << q[0] << " " << q[1] << " " << q[2] << " " << q[3] << std::endl;
    }
    f.close();
    std::cout << std::endl << "trajectory saved!" << std::endl;
}


/**
 * @brief 保存关键帧的轨迹
 * 轨迹包含关键帧的旋转矩阵和平移向量
 * 旋转矩阵以四元数的形式保存
 */
void System::SaveKeyFrameTrajectoryTUM(const std::string &filename)
{
    std::cout << std::endl << "Saving keyframe trajectory to " << filename << " ..." << std::endl;

    std::vector<KeyFrame*> vpKFs = mpMap->GetAllKeyFrames();
    std::sort(vpKFs.begin(), vpKFs.end(), KeyFrame::lId);

    // Transform all keyframes so that the first keyframe is at the origin.
    // After a loop closure the first keyframe might not be at the origin.
    // cv::Mat Two = vpKFs[0]->GetPoseInverse();

    std::ofstream f;
    f.open(filename.c_str());
    f << std::fixed;    // 当使用f<<std::fixed时，它指示输出流f将其后输出的所有浮点数都以‌固定小数点表示法进行格式化，而不是使用科学计数法（例如 1.234e+02）
                        // 这个操作符通常与 std::setprecision 结合使用，以精确控制输出浮点数的小数位数。例如，std::setprecision(2) 在 std::fixed 生效时，表示保留两位小数

    for (size_t i = 0; i < vpKFs.size(); i++) {
        KeyFrame* pKF = vpKFs[i];

        if (pKF->isBad()) {
            continue;
        }

        cv::Mat R = pKF->GetRotation().t();
        std::vector<float> q = Converter::toQuaternion(R);
        cv::Mat t = pKF->GetCameraCenter();
        f << std::setprecision(6) << pKF->mTimeStamp << std::setprecision(7) << " " << t.at<float>(0) << " " << t.at<float>(1) << " " << t.at<float>(2)
          << " " << q[0] << " " << q[1] << " " << q[2] << " " << q[3] << std::endl;
    }
    f.close();
    std::cout << std::endl << "trajectory saved!" << std::endl;
}


/**
 * @brief 保存KITTI数据集的轨迹
 * 与SaveTrajectoryTUM的区别：
 *  1. 不考虑遍历当前帧的跟踪状态 mpTracker->mlbLost
 *  2. 保存的结果，直接保存旋转矩阵，而不是保存旋转矩阵的四元数表示
 */
void System::SaveTrajectoryKITTI(const std::string &filename)
{
    std::cout << std::endl << "Saving camera trajectory to " << filename << " ..." << std::endl;
    if (mSensor == MONOCULAR) {
        std::cerr << "ERROR: SaveTrajectoryKITTI cannot be used for monocular." << std::endl;
        return;
    }

    std::vector<KeyFrame*> vpKFs = mpMap->GetAllKeyFrames();
    std::sort(vpKFs.begin(), vpKFs.end(), KeyFrame::lId);

    cv::Mat Two = vpKFs[0]->GetPoseInverse();   // Twc

    std::ofstream f;
    f.open(filename.c_str());
    f << std::fixed;

    // 帧位姿是相对于其参考关键帧(通过BA优化和位姿图优化)存储的。 获取时需要先提取关键帧位姿，再叠加相对变换。
    std::list<KeyFrame*>::iterator lRit = mpTracker->mlpReferences.begin(); // 参考帧
    std::list<double>::iterator lT = mpTracker->mlFrameTimes.begin();       // 时间戳
    for (std::list<cv::Mat>::iterator lit = mpTracker->mlRelativeFramePoses.begin(), lend = mpTracker->mlRelativeFramePoses.end();
         lit != lend;
         lit++, lRit++, lT++) {
        
        KeyFrame* pKF = *lRit;  // 获取参考帧

        cv::Mat Trw = cv::Mat::eye(4, 4, CV_32F);   // 将每个参考帧作为原点帧，目的是：当前参考帧有可能被剔除，就需要获取到一帧合适的关键帧，记录的是父关键帧到参考帧的位姿变换
        
        // 如果参考关键帧被剔除，则遍历 spanning tree（生成树）以获取一个合适的关键帧。
        while (pKF->isBad()) {
            Trw = Trw * pKF->mTcp;
            pKF = pKF->GetParent();
        }

        Trw = Trw * pKF->GetPose() * Two;   // Trw是参考帧的父关键帧到参考帧的位姿变换，pKF->GetPose是世界坐标系到参考帧的父关键帧之间的位姿变换，Two是选择的原点帧到世界坐标系的位姿变换
                                            // 所以结果就是原点帧到参考帧之间的位姿变换

        cv::Mat Tcw = (*lit) * Trw;
        cv::Mat Rwc = Tcw.rowRange(0, 3).colRange(0, 3).t();
        cv::Mat twc = -Rwc * Tcw.rowRange(0, 3).col(3);

        f << setprecision(9) << Rwc.at<float>(0,0) << " " << Rwc.at<float>(0,1)  << " " << Rwc.at<float>(0,2) << " "  << twc.at<float>(0) << " " <<
             Rwc.at<float>(1,0) << " " << Rwc.at<float>(1,1)  << " " << Rwc.at<float>(1,2) << " "  << twc.at<float>(1) << " " <<
             Rwc.at<float>(2,0) << " " << Rwc.at<float>(2,1)  << " " << Rwc.at<float>(2,2) << " "  << twc.at<float>(2) << endl;

    }
    f.close();
    std::cout << std::endl << "trajectory saved!" << std::endl;
}


void System::Reset()
{
    std::unique_lock<std::mutex> lock(mMutexReset);
    mbReset = true;
}
}