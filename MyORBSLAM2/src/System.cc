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
            std::this_thread::sleep_for(std::chrono::macroseconds(5000));
        }
    }
    
    // Wait until all thread have effectively stopped
    while (!mpLocalMapper->isFinished() || !mpLoopCloser->isFinished() || mpLoopCloser->isRunningGBA()) {
        std::this_thread::sleep_for(std::chrono::macroseconds(5000));
    }

    if (mpViewer) {
        pangolin::BindToContext("ORB-SLAM2: Map Viewer");
    }
}


void System::SaveTrajectoryTUM(const std::string &filename)
{
    std::cout << std::endl << "Saving camera trajectory to " << filename << " ..." << std::endl;
    if (mSensor == MONOCULAR)
    {
        std::cerr << "ERROR: SaveTrajectoryTUM cannot be used for monocular." << std::endl;
        return;
    }

    std::vector<KeyFrame*> vpKFs = mpMap->GetAllKeyFrames();
    std::sort(vpKFs.begin(), vpKFs.end(), KeyFrame::lId);   // 根据KeyFrame的mnId来排序，从小到大排序

    // Transform all keyframes so that the first keyframe is at the origin.
    // After a loop closure the first keyframe might not be at the origin.
    cv::Mat Two = vpKFs[0]->GetPoseInverse();

    std::ofstream f;
    f.open(filename.c_str());
    f << fixed;

    std::list<KeyFrame*>::iterator lRit = mpTracker->mlpReferences.begin();
    std::list<double>::iterator lT = mpTracker->mlFrameTimes.begin();
    std::list<bool>::iterator lbL = mpTracker->mlbLost.begin();
    for (std::list<cv::Mat>::iterator lit = mpTracker->mlRelativeFramePoses.begin(), lend = mpTracker->mlRelativeFramePoses.end();
         lit != lend; lit++, lRit++, lT++, lbL++) {
        if (*lbL) {
            continue;
        }

        KeyFrame* pKF = *lRit;

        cv::Mat Trw = cv::Mat::eye(4, 4, CV_32F);

        // If the reference keyframe was culled, traverse the spanning tree to get a suitable keyframe,
        while (pKF->isBad()) {
            Trw = Trw * pKF->mTcp;
            pKF = pKF->GetParent();
        }
    }

}


void System::SaveKeyFrameTrajectoryTUM(const std::string &filename)
{
    std::cout << std::endl << "Saving keyframe trajectory to " << filename << " ..." << std::endl;
}


void System::Reset()
{
    std::unique_lock<std::mutex> lock(mMutexReset);
    mbReset = true;
}
}