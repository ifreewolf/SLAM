#include "Tracking.h"

namespace ORB_SLAM2
{

Tracking::Tracking(System* pSys,
                   ORBVocabulary* pVoc,
                   FrameDrawer* pFrameDrawer,
                   MapDrawer* pMapDrawer,
                   Map* pMap,
                   KeyFrameDatabase* pKFDB,
                   const std::string &strSettingPath,
                   const int sensor):
                        mState(NO_IMAGES_YET),
                        mSensor(sensor),
                        mbOnlyTracking(false),
                        mpORBVocabulary(pVoc),
                        mpKeyFrameDB(pKFDB),
                        mpInitializer(static_cast<Initializer*>(NULL)),
                        mpSystem(pSys),
                        mpFrameDrawer(pFrameDrawer),
                        mpMapDrawer(pMapDrawer),
                        mpMap(pMap)

{
    cv::FileStorage fSettings(strSettingPath, cv::FileStorage::READ);
    float fx = fSettings["Camera.fx"];
    float fy = fSettings["Camera.fy"];
    float cx = fSettings["Camera.cx"];
    float cy = fSettings["Camera.cy"];

    // 内参矩阵如下：
    //     |fx 0 cx|
    // K = |0 fy cy|
    //     |0  0  1|
    // 构造相机内参矩阵
    cv::Mat K = cv::Mat::eye(3, 3, CV_32F);
    K.at<float>(0, 0) = fx;
    K.at<float>(1, 1) = fy;
    K.at<float>(0, 2) = cx;
    K.at<float>(1, 2) = cy;
    K.copyTo(mK);

    // 图像矫正系数
    //[k1 k2 p1 p2 k3]
    cv::Mat DistCoef(4, 1, CV_32F);
    DistCoef.at<float>(0) = fSettings["Camera.k1"];
    DistCoef.at<float>(1) = fSettings["Camera.k2"];
    DistCoef.at<float>(2) = fSettings["Camera.p1"];
    DistCoef.at<float>(3) = fSettings["Camera.p2"];
    const float k3 = fSettings["Camera.k3"];
    // 有些相机的畸变系数中没有k3
    if (k3 != 0) {
        DistCoef.resize(5);
        DistCoef.at<float>(4) = k3;
    }
    DistCoef.copyTo(mDistCoef);

    // 双目摄像头 baseline * fx = 40
    mbf = fSettings["Camera.bf"];

    float fps = fSettings["Camera.fps"];
    if (fps == 0) {
        fps = 30;
    }

    // Max/Min Frames to insert keyframes and to check relocalisation
    mnMinFrames = 0;
    mnMaxFrames = fps;

    std::cout << std::endl << "Camera Parameters:" << std::endl;
    std::cout << "- fx: " << fx << std::endl;
    std::cout << "- fy: " << fy << std::endl;
    std::cout << "- cx: " << cx << std::endl;
    std::cout << "- cy: " << cy << std::endl;
    std::cout << "- k1: " << DistCoef.at<float>(0) << std::endl;
    std::cout << "- k2: " << DistCoef.at<float>(1) << std::endl;
    if (DistCoef.rows == 5) {
        std::cout << "- k3: " << DistCoef.at<float>(4) << std::endl;
    }
    std::cout << "- p1: " << DistCoef.at<float>(2) << std::endl;
    std::cout << "- p2: " << DistCoef.at<float>(3) << std::endl;
    std::cout << "- fps: " << fps << std::endl;

    // 1:RGB 0:BGR
    int nRGB = fSettings["Camera.RGB"];
    mbRGB = nRGB;

    if (mbRGB) {
        std::cout << "- color order: RGB (ignored if grayscale)" << std::endl;
    } else {
        std::cout << "- color order: BGR (ignored if grayscale)" << std::endl;
    }

    // Load ORB parameters
    // Step2. 加载ORB特征点有关的参数，并新建特征点提取器
    // 每一帧图提取的特征点数量 1000
    int nFeatures = fSettings["ORBextractor.nFeatures"];
    // 图像建立金字塔时的变化尺度 1.2
    float fScaleFactor = fSettings["ORBextractor.scaleFactor"];
    // 尺度金字塔的层数 8
    int nLevels = fSettings["ORBextractor.nLevels"];
    // 提取fast特征点的默认阈值 20
    int fIniThFAST = fSettings["ORBextractor.iniThFAST"];
    // 如果默认阈值提取不出足够fast特征点，则使用最小阈值 8
    int fMinThFAST = fSettings["ORBextractor.minThFAST"];


    // 创建两个ORB特征点提取器
    // tracking过程都会用到 mpORBextractorLeft 作为特征点提取器
    mpORBextractorLeft = new ORBextractor(nFeatures, fScaleFactor, nLevels, fIniThFAST, fMinThFAST);

    // 如果是双目，tracking过程中还会用到mpORBextractorRight作为右目特征点提取器
    if (sensor == System::STEREO) {
        mpORBextractorRight = new ORBextractor(nFeatures, fScaleFactor, nLevels, fIniThFAST, fMinThFAST);
    }

    // 在单目初始化的时候，会用mpIniORBextractor来作为特征点提取器，特征点数量翻倍
    // mpIniORBextractor 与 mpORBextractorLeft、mpORBextractorRight 的不同，主要在于 _nfeatures 参数的不同，也就是特征提取的数目不一样。
    if (sensor == System::MONOCULAR) {
        mpIniORBextractor = new ORBextractor(2*nFeatures, fScaleFactor, nLevels, fIniThFAST, fMinThFAST);
    }

    if (sensor == System::STEREO || sensor == System::RGBD) {
        // 判断一个3D点远/近的阈值 mbf * 35 / fx
        // ThDepth其实就是表示极限长度的多少倍
        mfThDepth = mbf*(float)fSettings["ThDepth"] / fx;
        std::cout << std::endl << "Depth Threshold (Close/Far Points): " << mfThDepth << std::endl;
    }

    if (sensor == System::RGBD) {
        // 深度相机disparity转化为depth时的因子，disparity就是视差
        mDepthMapFactor = fSettings["DepthMapFactor"];
        if (fabs(mDepthMapFactor) < 1e-5) {
            mDepthMapFactor = 1;
        } else {
            mDepthMapFactor = 1.0f / mDepthMapFactor;
        }

    }

}


cv::Mat Tracking::GrabImageRGBD(const cv::Mat &imRGB, const cv::Mat &imD, const double &timestamp)
{
    mImGray = imRGB;
    cv::Mat imDepth = imD;

    if (mImGray.channels() == 3) {
        if (mbRGB) {
            cv::cvtColor(mImGray, mImGray, cv::COLOR_RGB2GRAY);
        } else {
            cv::cvtColor(mImGray, mImGray, cv::COLOR_BGR2GRAY);
        }
    } else if (mImGray.channels() == 4) {
        if (mbRGB) {
            cv::cvtColor(mImGray, mImGray, cv::COLOR_RGBA2GRAY);
        } else {
            cv::cvtColor(mImGray, mImGray, cv::COLOR_BGRA2GRAY);
        }
    }

    if ((fabs(mDepthMapFactor - 1.0f) > 1e-5) || imDepth.type() != CV_32F) {
        imDepth.convertTo(imDepth, CV_32F, mDepthMapFactor);    // 这里相当于做了一个深度的恢复
        // void convertTo(OutputArray dst, int rtype, double alpha=1, double beta=0) const;
        // dst‌：输出矩阵，尺寸和通道数与输入相同，数据类型由rtype指定
        // rtype‌：目标数据类型（如CV_8U、CV_32F），负值表示保持输入类型
        // alpha‌：缩放因子（默认1.0），用于调整对比度
        // beta‌：偏移量（默认0.0），用于调整亮度
        
        // 所以这里的功能就是将imDepth深度图缩放一定的倍数

    }

    mCurrentFrame = Frame(mImGray, imDepth, timestamp, mpORBextractorLeft, mpORBVocabulary, mK, mDistCoef, mbf, mfThDepth);

    Track();

    return mCurrentFrame.mTcw.clone();
}


void Tracking::SetLocalMapper(LocalMapping* pLocalMapper)
{
    mpLocalMapper = pLocalMapper;
}


void Tracking::SetLoopClosing(LoopClosing* pLoopClosing)
{
    mpLoopClosing = pLoopClosing;
}

void Tracking::SetViewer(Viewer* pViewer)
{
    mpViewer = pViewer;
}

void Tracking::InformOnlyTracking(const bool &flag)
{
    mbOnlyTracking = flag;
}

void Tracking::Reset()
{
    std::cout << "Tracking Reseting" << std::endl;
    if (mpViewer) {
        mpViewer->RequestStop();
        while (!mpViewer->isStopped()) {
            std::this_thread::sleep_for(std::chrono::microseconds(3000));
        }

        // Reset Local Mapping
        std::cout << "Reseting Local Mapper...";
        mpLocalMapper->RequestReset();
        std::cout << " done" << std::endl;

        // Reset Loop Closing
        std::cout << "Reseting Loop Closing...";
        mpLoopClosing->RequestReset();
        std::cout << " done" << std::endl;

        // Clear BoW Database
        std::cout << "Reseting Database...";
        mpKeyFrameDB->clear();
        std::cout << " done" << std::endl;

        // Clear Map (this erase MapPoints and KeyFrames)
        mpMap->clear();

        KeyFrame::nNextId = 0;
        Frame::nNextId = 0;
        mState = NO_IMAGES_YET;

        if (mpInitializer) {
            delete mpInitializer;
            mpInitializer = static_cast<Initializer*>(NULL);
        }

        mlRelativeFramePoses.clear();
        mlpReferences.clear();
        mlFrameTimes.clear();
        mlbLost.clear();

        if (mpViewer) {
            mpViewer->Release();
        }


    }
}

void Tracking::Track()
{
    // Track 包含两部分：估计运动、根据局部地图

    // mState为tracking的状态，包括 SYSTEM_NOT_READY, NO_IMAGE_YET, NOT_INITIALIZED, OK, LOST
    if (mState == NO_IMAGES_YET) {
        mState = NOT_INITIALIZED;
    }

    // mLastProcessedState 存储了Tracking最新的状态，用于FrameDrawer中的绘制
    mLastProcessedState = mState;

    std::unique_lock<std::mutex> lock(mpMap->mMutexMapUpdate);

    std::cout << "fangguanshou mState = " << mState << std::endl;

    if (mState == NOT_INITIALIZED) {
        if (mSensor == System::STEREO || mSensor == System::RGBD) {
            StereoInitialization();     // 双目RGBD相机的初始化共用同一函数
        } else {
            MonocularInitialization();  // 单目初始化
        }

        // 更新帧绘制器中存储的最新状态
        mpFrameDrawer->Update(this);

        // 这个状态量在上面的初始化函数中被更新
        if (mState != OK) {
            std::cout << "fangguanshou initialization failed." << std::endl;
            return;
        }
    } else {
        // System is initialized. Track Frame.
        // bOk为临时变量，用于表示每个函数是否执行成功
        bool bOK;

        std::cout << "fangguanshou mbOnlyTracking = " << mbOnlyTracking << std::endl;

        // Initial camera pose estimation using motion model or relocalization (if tracking is lost)
        // mbOnlyTracking等于false表示正常SLAM模式（定位+地图更新），mbOnlyTracking等于true表示仅定位模式
        // Tracking 类构造时默认为false，在viewer中有个开关ActivateLocalizationMode，可以控制是否开启mbOnlyTracking
        if (!mbOnlyTracking) {  // SLAM模式
            // Local Mapping is activated. This is the normal behaviour, unless
            // you explicitly activate the "only tracking" mode.

            std::cout << "fangguanshou in SLAM model, mState = " << mState << std::endl;

            // Step2 跟踪进入正常SLAM模式，有地图更新
            // 是否正常跟踪
            if (mState == OK) {
                // Loal Mapping might have changed some MapPoints tracked in last frame
                // Step2.1 检查并更新上一帧被替换的MapPoints
                // 局部建图线程则可能会对原有的地图点进行替换，在这里进行检查
                CheckReplacedInLastFrame();

                // Step2.2 运动模型是空的或刚完成重定位，跟踪参考关键帧；否则恒速模型跟踪
                // 第一个条件，如果运动模型为空，说明刚初始化开始，或者已经跟丢了
                // 第二个条件，如果当前帧紧紧地跟着在重定位的帧的后面，我们将重定位帧来恢复位姿
                // mnLastRelocFrameId 上一次重定位的那一帧
                // mVelocity.empty() 表示运动模型为空；mCurrentFrame.mnId < mnLastRelocFrameId + 2表示刚完成重定位
                if (mVelocity.empty() || mCurrentFrame.mnId < mnLastRelocFrameId + 2) {
                    std::cout << "fangguanshou Track TrackReferenceKeyFrame." << std::endl;
                    // 用最近的关键帧来跟踪当前的普通帧
                    // 通过BoW的方式在参考帧中找到当前帧特征点的匹配点
                    // 优化每个特征点所对应3D点重投影误差即可得到位姿
                    bOK = TrackReferenceKeyFrame(); // 跟踪参考关键帧
                    std::cout << "fangguanshou Track TrackReferenceKeyFrame. bOK = " << bOK << std::endl;
                } else {
                    std::cout << "fangguanshou Track TrackWithMotionModel." << std::endl;
                    // 用最近的普通帧来跟踪当前的普通帧
                    // 根据恒速模型设定当前帧的初始位姿
                    // 通过投影的方式在参考帧中找到当前帧特征点的匹配点
                    // 优化每个特征点所对应3D点的投影误差即可得到位姿
                    bOK = TrackWithMotionModel();
                    std::cout << "fangguanshou Track TrackWithMotionModel. bOK = " << bOK << std::endl;
                    if (!bOK) {
                        // 如果恒速模型失败了，只能根据参考帧来跟踪
                        std::cout << "fangguanshou Track TrackWithMotionModel faild. change to TrackReferenceKeyFrame." << std::endl;
                        bOK = TrackReferenceKeyFrame();
                        std::cout << "fangguanshou Track TrackWithMotionModel faild. change to TrackReferenceKeyFrame. bOK = " << bOK << std::endl;
                    }
                }
            } else {    // 如果mState状态不为OK
                // 如果跟踪状态不成功，那么就只能重定位了
                // BoW搜索，EPnP求解位姿
                bOK = Relocalization(); // 重定位跟踪
            }
        } else {    // only Tracking Mode
            // Localization Mode: Loal Mapping is deactivated
            // Step2 只进行跟踪Tracking，局部地图不工作
            if (mState == LOST) {
                // Step2.1 如果跟丢了，只能重定位
                bOK = Relocalization();
            } else {
                // mbVO是mbOnlyTracking为true时才有的一个变量
                // mbVO为false表示此帧匹配了很多的MapPoints，跟踪很正常
                // mbVO为true表示此帧匹配了很少MapPoints，少于10个
                if (!mbVO) {    // 匹配了很多MapPoints
                    // In last frame we tracked enough MapPoints in the map
                    // Step2.2 如果跟踪正常，使用恒速模型 或 参考关键帧跟踪
                    if (!mVelocity.empty()) {
                        bOK = TrackWithMotionModel();
                        if (!bOK) {
                            // 如果恒速模型失败，只能根据参考帧来跟踪
                            bOK = TrackReferenceKeyFrame();
                        }
                    } else {    // 如果恒速模型不被满足，那么就只能够通过参考关键帧来定位
                        bOK = TrackReferenceKeyFrame();
                    }
                } else {    // 匹配了很少的MapPoints
                    // In last frame we tracked mainly "visual odometry" points.    在上一帧中，我们主要跟踪了"视觉里程计"特征点

                    // We compute two camera poses, one from motion model and one doing relocalization. 我们计算两个相机位姿：一个来自运动模型，另一个通过重定位获得。
                    // If relocalization is sucessfull we choose that solution, otherwise we retain     如果重定位成功，则选择该方案；否则保留"视觉里程计"的位姿解。
                    // the "visual odometry" solution.

                    // mvVO为true表明此帧匹配了很少(小于10)的地图点
                    // MM=Motion Mode，通过运动模型进行跟踪的结果
                    bool bOKMM = false;
                    // 通过重定位方法来跟踪的结果
                    bool bOKReloc = false;

                    // 运动模型中构造的地图点
                    std::vector<MapPoint*> vpMPsMM; // 运动模型中构造的地图点
                    std::vector<bool> vbOutMM;      // 在追踪运动模型后发现的外点
                    cv::Mat TcwMM;                  // 运动模型得到的位姿

                    // Step2.3 当运动模型有效的时候，根据运动模型计算位姿
                    if (!mVelocity.empty()) {
                        bOKMM = TrackWithMotionModel();

                        // 将恒速模型跟踪结果暂存到这几个变量中，因为后面重定位会改变这些变量
                        vpMPsMM = mCurrentFrame.mvpMapPoints;
                        vbOutMM = mCurrentFrame.mvbOutlier;
                        TcwMM   = mCurrentFrame.mTcw.clone();
                    }

                    // Step2.4 使用重定位的方法来得到当前帧的位姿
                    bOKReloc = Relocalization();

                    // Step2.5 根据前面的恒速模型，重定位结果来更新状态
                    if (bOKMM && !bOKReloc) {
                        // 恒速模型成功、重定位失败，重新使用之前暂存的恒速模型结果
                        mCurrentFrame.SetPose(TcwMM);
                        mCurrentFrame.mvpMapPoints = vpMPsMM;
                        mCurrentFrame.mvbOutlier = vbOutMM;

                        // 如果当前帧匹配的3D点很少，增加当前可视地图点的被观测次数
                        if (mbVO) {
                            // 更新当前帧的地图点被观测次数
                            for (int i = 0; i < mCurrentFrame.N; i++) {
                                // 如果这个特征点形成了地图点，并且也不是外点的时候
                                if (mCurrentFrame.mvpMapPoints[i] && !mCurrentFrame.mvbOutlier[i]) {
                                    // 增加能观测到该地图点的帧数
                                    mCurrentFrame.mvpMapPoints[i]->IncreaseFound();
                                }
                            }
                        }
                    } else if (bOKReloc) {
                        // 只要重定位成功整个跟踪过程正常进行（重定位与跟踪，更相信重定位）
                        mbVO = false;   // 表示匹配了很多的MapPoints
                    }
                    // 有一个成功，我们就认为执行成功了
                    bOK = bOKReloc || bOKMM;
                }
            }
        }

        // 将最新的关键帧作为当前帧的参考关键帧
        mCurrentFrame.mpReferenceKF = mpReferenceKF;

        std::cout << "fangguanshou after Track mbOnlyTracking = " << mbOnlyTracking << ", bOK = " << bOK << std::endl;

        // If we have an initial estimation of the camera pose and matching. Track the local map.
        // Step3 在跟踪得到当前帧初始姿态后，现在对local map进行跟踪得到更多的匹配，并优化当前位姿
        // 前面只是跟踪一帧得到初始位姿，这里搜索局部关键帧、局部地图点，和当前帧进行投影匹配，得到更多匹配的MapPoint后进行Pose优化
        if (!mbOnlyTracking) {  // SLAM模式
            std::cout << "fangguanshou in SLAM model, bOK = " << bOK << std::endl;
            if (bOK) {
                std::cout << "fangguanshou in SLAM model, bOK = yes, start TrackLocalMap." << std::endl;
                bOK = TrackLocalMap();
                std::cout << "fangguanshou in SLAM model, bOK = yes, start TrackLocalMap. bOK = " << bOK << std::endl;
            }
        } else {    // 仅跟踪模式
            
            // mbVO true means that there are few matches to MapPoints int the map.     mbVO（运动模型视觉里程计）为true表示当前帧与地图中的MapPoints匹配点较少
            // We cannot retrieve a local map and therefore we do not perform TrackLocalMap().  系统无法获取局部地图，因此不会执行TrackLocalMap()操作。
            // Once the system relocalizes the camera we will use the local map again.          一旦相机重定位成功，系统将重新启用局部地图功能。

            // 重定位成功
            // !mbVO， 当mbVO为false，表示此帧匹配了很多的MapPoint
            if (bOK && !mbVO) {
                bOK = TrackLocalMap();
            }
        }

        std::cout << "fangguanshou after TrackLocalMap, bOK = " << bOK << std::endl;

        // 根据上面的操作来判断是否跟踪成功
        if (bOK) {
            mState = OK;
        } else {
            mState = LOST;
        }

        std::cout << "fangguanshou after TrackLocalMap, mState = " << mState << std::endl;

        // Update drawer
        // Step4 更新显示线程中的图像、特征点、地图点信息
        mpFrameDrawer->Update(this);

        // If tracking were good, check if we insert a keyframe
        // 只有在成功追踪时才考虑生成关键帧的问题
        if (bOK) {
            // Update motion model
            // Step5 跟踪成功，更新恒速运动模型
            if (!mLastFrame.mTcw.empty()) {
                // 更新恒速运动模型，TrackWithMotionModel 中的 mVelocity
                cv::Mat LastTwc = cv::Mat::eye(4, 4, CV_32F);
                mLastFrame.GetRotationInverse().copyTo(LastTwc.rowRange(0, 3).colRange(0, 3));
                mLastFrame.GetCameraCenter().copyTo(LastTwc.rowRange(0, 3).col(3));
                // mVelocity = Tcl = Tcw*Twl，表示上一帧到当前帧的变换，其中Twl = LastTwc
                mVelocity = mCurrentFrame.mTcw * LastTwc;
            } else {
                // 否则速度为空
                mVelocity = cv::Mat();
            }

            // 更新显示中的位姿
            mpMapDrawer->SetCurrentCameraPose(mCurrentFrame.mTcw);

            // Clean VO matches
            // Step6 清除观测不到的地图点
            for (int i = 0; i < mCurrentFrame.N; i++) {
                MapPoint* pMP = mCurrentFrame.mvpMapPoints[i];
                if (pMP) {
                    if (pMP->Observations() < 1) {
                        mCurrentFrame.mvbOutlier[i] = false;
                        mCurrentFrame.mvpMapPoints[i] = static_cast<MapPoint*>(NULL);
                    }
                }
            }

            // Delete temporal MapPoints
            // Step7 清除恒速模型跟踪中 UpdateLastFram e中为当前帧临时添加的MapPoints（仅双目和RGBD）
            for (std::list<MapPoint*>::iterator lit = mlpTemporalPoints.begin(), lend = mlpTemporalPoints.end(); lit != lend; lit++) {
                MapPoint* pMP = *lit;
                delete pMP;
            }

            // 这里不仅仅是清除mlpTemporalPoints，通过delete pMP还删除了指针指向的MapPoint
            // 不能够直接执行这个是因为其中存储的都是指针，之前的操作都是为了避免内存泄漏
            mlpTemporalPoints.clear();

            // 若跟踪成功，根据条件判定是否产生关键帧
            // Check if we need to insert a new keyframe
            // Step8 检测并插入关键帧，对于双目活RGBD会产生新的地图点
            if (NeedNewKeyFrame()) {
                CreateNewKeyFrame();
            }

            // We allow points with high innovation (considered outliers by the Huber Function)
            // pass to the new keyframe, so that bundle adjustment will finally decide
            // if they are outliers or not. We don't want next frame to estimate its position
            // with those points so we discard them in the frame.
            // 

            // Step9 删除那些在bundle adjustment中检测为outlier的地图点
            for (int i = 0; i < mCurrentFrame.N; i++) {
                // 这里第一个条件还要执行判断是因为，前面的操作中可能删除了其中的地图点
                if (mCurrentFrame.mvpMapPoints[i] && mCurrentFrame.mvbOutlier[i]) {
                    mCurrentFrame.mvpMapPoints[i] = static_cast<MapPoint*>(NULL);
                }
            }
        }

        // Reset if the camera get lost soom after initialization
        // Step10 如果初始化不久就跟踪失败，并且relocation也没有搞定，只能重新Reset
        if (mState == LOST) {
            // 如果地图中的关键帧信息过少的话，直接重新进行初始化了
            if (mpMap->KeyFramesInMap() <= 5) {
                std::cout << "Track lost soon after initialisation, reseting..." << std::endl;
                mpSystem->Reset();
                return;
            }
        }

        // 确保已经设置了参考关键帧
        if (!mCurrentFrame.mpReferenceKF) {
            mCurrentFrame.mpReferenceKF = mpReferenceKF;
        }

        // 保存上一帧的数据，当前帧变上一帧
        mLastFrame = Frame(mCurrentFrame);
    }

    // Store frame pose information to retrieve the complete camera trajectory afterwards.
    // Step11. 记录位姿信息，用于最后保存所有的轨迹
    if (!mCurrentFrame.mTcw.empty()) {
        // 计算相对位姿 Tcr = Tcw * Twr, Twr = Trw^-1
        cv::Mat Tcr = mCurrentFrame.mTcw * mCurrentFrame.mpReferenceKF->GetPoseInverse();
        // 保存各种状态
        mlRelativeFramePoses.push_back(Tcr);
        mlpReferences.push_back(mpReferenceKF);
        mlFrameTimes.push_back(mCurrentFrame.mTimeStamp);
        mlbLost.push_back(mState == LOST);
    } else {
        // This can happen if tracking is lost
        // 如果跟踪失败，则相对位姿使用上一次值
        mlRelativeFramePoses.push_back(mlRelativeFramePoses.back());
        mlpReferences.push_back(mlpReferences.back());
        mlFrameTimes.push_back(mlFrameTimes.back());
        mlbLost.push_back(mState == LOST);
    }
}


void Tracking::StereoInitialization()
{
    if (mCurrentFrame.N > 500) {
        // Set Frame pose to the origin
        mCurrentFrame.SetPose(cv::Mat::eye(4, 4, CV_32F));

        // Create KeyFrame
        KeyFrame* pKFini = new KeyFrame(mCurrentFrame, mpMap, mpKeyFrameDB);

        pKFini->ComputeBow();

        // Insert KeyFrame in the map
        mpMap->AddKeyFrame(pKFini);
        
        // Create MapPoints and associate to KeyFrame
        for (int i = 0; i < mCurrentFrame.N; i++) {
            float z = mCurrentFrame.mvDepth[i];
            if (z > 0) {
                cv::Mat x3D = mCurrentFrame.UnprojectStereo(i);         // 第i个特征点在世界坐标系下的三维坐标
                MapPoint* pNewMP = new MapPoint(x3D, pKFini, mpMap);    // 根据特征点的三维坐标创建地图点，是在世界坐标系下
                pNewMP->AddObservation(pKFini, i);                      // 地图点添加观察帧
                pKFini->AddMapPoint(pNewMP, i);                         // 关键帧添加地图点
                pNewMP->ComputeDistinctiveDescriptors();                // 地图点重新计算平均描述子
                pNewMP->UpdateNormalAndDepth();                         // 地图点重新计算深度和方向
                mpMap->AddMapPoint(pNewMP);                             // 地图添加地图点

                mCurrentFrame.mvpMapPoints[i] = pNewMP;                 // 当前帧地图点赋值
            }
        }

        std::cout << "New map created with " << mpMap->MapPointsInMap() << " points" << std::endl;

        mpLocalMapper->InsertKeyFrame(pKFini);  // LocalMapping线程添加关键帧，LocalMapping线程会使用关键帧进行全局Bundle Adjustment

        mLastFrame = Frame(mCurrentFrame);      // 记录最新帧
        mnLastKeyFrameId = mCurrentFrame.mnId;  // 记录最新关键帧的Id
        mpLastKeyFrame = pKFini;                // 记录最新关键帧

        mvpLocalKeyFrames.push_back(pKFini);            // 局部关键帧列表，添加当前关键帧
        mvpLocalMapPoints = mpMap->GetAllMapPoints();   // 获取地图上所有地图点
        mpReferenceKF = pKFini;                         // 参考关键帧，初始化成功的帧会被设为参考关键帧
        mCurrentFrame.mpReferenceKF = pKFini;           // 当前帧的参考关键帧就是初始化成功的关键帧

        mpMap->SetReferenceMapPoints(mvpLocalMapPoints);        // 设置地图的参考地图点

        mpMap->mvpKeyFrameOrigins.push_back(pKFini);            // 地图的关键帧列表添加初始化成功的关键帧

        mpMapDrawer->SetCurrentCameraPose(mCurrentFrame.mTcw);  // 地图绘制器设置当前相机位姿

        mState = OK;    // 修改Tracking状态为OK
    }
}


void Tracking::MonocularInitialization()
{
    std::cout << "fangguanshou mono" << std::endl;
    return;
}


/**
 * @brief 判断当前帧是否需要插入关键帧
 * 
 */
bool Tracking::NeedNewKeyFrame()
{
    // Step1 纯VO模式下不插入关键帧
    if (mbOnlyTracking) {
        return false;
    }

    // If Local Mapping is freezed by a Loop Closure do not insert keyframes
    // Step2 如果局部地图线程被闭环检测线程使用，则不插入关键帧
    if (mpLocalMapper->isStopped() || mpLocalMapper->stopRequested()) {
        return false;
    }

    // 获取当前地图中的关键帧数目
    const int nKFs = mpMap->KeyFramesInMap();

    // Step3 如果距离上一次重定位比较近时，关键帧数目超出最大限制，不插入关键帧
    // mCurrentFrame.mnId 是当前帧的ID
    // mnLastRelocFrameId 是最近一次重定位的ID
    // mnMaxFrames 等于图像输入的帧率
    if (mCurrentFrame.mnId < mnLastRelocFrameId + mnMaxFrames && nKFs > mnMaxFrames) {
        return false;
    }

    // Step4 得到参考帧跟踪到的地图点数量
    // UpdateLocalKeyFrames 函数中会将与当前帧共视程度最高的关键帧设定为当前帧的参考关键帧

    // 地图点的最小观测次数
    int nMinObs = 3;
    if (nKFs <= 2) {
        nMinObs = 2;
    }
    // 参考关键帧地图点中观测到的数目 >= nMinObs 的地图点数目
    int nRefMatches = mpReferenceKF->TrackedMapPoints(nMinObs);


    // Step5 查询局部地图线程是否繁忙，当前能否接受新的关键帧
    bool bLocalMappingIdle = mpLocalMapper->AcceptKeyFrames();

    // Step6 对于双目或RGBD摄像头，统计成功跟踪的近点的数量，如果跟踪到的近点太少，没有跟踪到的近点较多，可以插入关键帧
    int nNonTrackedClose = 0;   // 双目或RGBD中没有跟踪到的近点
    int nTrackedClose = 0;      // 双目或RGBD中成功跟踪的近点(三维点)
    if (mSensor != System::MONOCULAR) {
        for (int i = 0; i < mCurrentFrame.N; i++) {
            // 深度值在有效范围内
            if (mCurrentFrame.mvDepth[i] > 0 && mCurrentFrame.mvDepth[i] < mfThDepth) {
                if (mCurrentFrame.mvpMapPoints[i] && !mCurrentFrame.mvbOutlier[i]) {
                    nTrackedClose++;
                } else {
                    nNonTrackedClose++;
                }
            }
        }
    }

    // 双目或RGBD情况下：跟踪到的地图点中近点太少，同时没有跟踪到的三维点太多，可以插入关键帧了
    // 单目时，为false
    bool bNeedToInsertClose = (nTrackedClose < 100) && (nNonTrackedClose > 70);

    // Step7 决策是否需要插入关键帧
    // Step7.1 设定比例阈值，当前帧和参考关键帧跟踪到点的比例，比例越大，越倾向于增加关键帧
    float thRefRatio = 0.75f;

    // 关键帧只有一帧，那么插入关键帧和参考帧的阈值设置的低一些，插入频率较低
    if (nKFs < 2) {
        thRefRatio = 0.4f;
    }

    // 单目情况下，插入关键帧的频率很高
    if (mSensor == System::MONOCULAR) {
        thRefRatio = 0.9f;
    }

    // Step7.2 很长时间没有插入关键帧，可以插入
    const bool c1a = mCurrentFrame.mnId >= mnLastKeyFrameId + mnMaxFrames;

    // Step7.3 满足插入关键帧的最小间隔并且localMapper处于空闲状态，可以插入
    const bool c1b = (mCurrentFrame.mnId >= mnLastKeyFrameId + mnMinFrames && bLocalMappingIdle);

    // Step7.4 在双目、RGBD的情况下当前帧跟踪到的点比参考帧关键帧的0.25倍还少，或者满足bNeedToInsertClose
    const bool c1c = mSensor != System::MONOCULAR &&            // 只考虑在双目、RGBD的情况
                     (mnMatchesInliers < nRefMatches * 0.25 ||  // 当前帧和地图点匹配的数目非常少
                      bNeedToInsertClose);                      // 需要插入
    
    // Step7.5 和参考帧相比当前帧跟踪到的点太少，或者满足bNeedToInsertClose: 同时跟踪到的内点还不能太少
    const bool c2 = ((mnMatchesInliers < nRefMatches * thRefRatio || bNeedToInsertClose) && mnMatchesInliers > 15);

    if ((c1a || c1b || c1c) && c2) {
        // Step7.6 Local Mapping空闲时可以直接插入，不空闲的时候要根据情况插入
        if (bLocalMappingIdle) {
            // 可以插入关键帧
            return true;
        } else {
            mpLocalMapper->InterruptBA();
            if (mSensor != System::MONOCULAR) {
                // 队列里不能阻塞太多关键帧
                // Tracking插入关键帧不是直接插入，而是先插入到mlNewKeyFrames中，然后mpLocalMapper再逐个pop出来插入到mspKeyFrames
                if (mpLocalMapper->KeyframesInQueue() < 3) {
                    // 队列中的关键帧数目不是很多，可以插入
                    return true;
                } else {
                    // 队列中缓冲的关键帧数目太多，暂时不能插入
                    return false;
                }
            } else {
                // 对于单目情况，就直接无法插入关键帧了
                // ？？？为什么这里对单目情况的处理不一样？
                // 回答：可能是因为单目关键帧相对比较密集
                return false;
            }
        }
    } else {
        // 不能满足上面的条件，自然不能插入关键帧
        return false;
    }


}


/**
 * @brief 创建新的关键帧
 * 对于非单目的情况，同时创建新的MapPoints
 * 
 * Step1 将当前帧构造成关键帧
 * Step2 将当前关键帧设置为当前帧的参考关键帧
 * Step3 对于双目或RGBD摄像头，为当前帧生成新的MapPoints
 */
void Tracking::CreateNewKeyFrame()
{
    // 如果局部建图线程关闭了，就无法插入关键帧
    if (!mpLocalMapper->SetNotStop(true)) {
        return;
    }

    // Step1 将当前帧构造成关键帧
    // 根据当前帧，全局地图，以及关键帧数据库创建一个关键帧
    KeyFrame* pKF = new KeyFrame(mCurrentFrame, mpMap, mpKeyFrameDB);

    // Step2 将当前关键帧设置为当前帧的参考关键帧
    // 在 UpdateLocalKeyFrames 函数中会将与当前关键帧共视程度最高的关键帧设定为当前帧的参考关键帧
    // 把创建出来的关键帧赋值为Tracking的参考关键帧，同时赋值给当前帧的参考关键帧（其会在 UpdateLocalKeyFrames 函数中被更新，替换成与该关键帧共视程度最高的关键帧）。
    mpReferenceKF = pKF;
    mCurrentFrame.mpReferenceKF = pKF;

    // Step3 对于双目或RGBD摄像头，为当前帧生成新的地图点，单目无操作
    // 这段代码和 Tracking::UpdateLastFrame 中的那一部分代码功能相同
    if (mSensor != System::MONOCULAR) {
        // 根据Tcw计算mRcw，mtcw和mRwc,mOw
        mCurrentFrame.UpdatePoseMatrices();

        // Step3.1 得到当前帧有深度值的特征点（不一定是地图点）
        std::vector<std::pair<float, int>> vDepthIdx;
        vDepthIdx.reserve(mCurrentFrame.N);
        for (int i = 0; i < mCurrentFrame.N; i++) {
            float z = mCurrentFrame.mvDepth[i];
            if (z > 0) {
                // 第一个元素是深度，第二个元素是对应的特征点的id
                vDepthIdx.push_back(std::make_pair(z,i));
            }
        }

        if (!vDepthIdx.empty()) {
            // Step3.2 按照深度从小到大排序
            std::sort(vDepthIdx.begin(), vDepthIdx.end());

            // Step3.3 从中找出不是地图点的生成临时地图点
            int nPoints = 0;
            for (size_t j = 0; j < vDepthIdx.size(); j++) {
                int i = vDepthIdx[j].second;
                bool bCreateNew = false;

                // 如果这个点对应在当前帧中的地图点没有，或者创建后就没有被观测到，那么就生成一个临时的地图点
                MapPoint* pMP = mCurrentFrame.mvpMapPoints[i];
                if (!pMP) {
                    bCreateNew = true;
                } else if (pMP->Observations() < 1) {   // 没有被观测到
                    bCreateNew = true;
                    mCurrentFrame.mvpMapPoints[i] = static_cast<MapPoint*>(NULL);
                }

                // 如果需要就新建地图点，这里的地图点不是临时的，是全局地图中新建地图点，用于跟踪
                if (bCreateNew) {
                    // 当某个特征点的深度信息或者双目信息有效时，将它反投影到三维世界坐标系中。计算出特征点在世界坐标系下的3D坐标。
                    cv::Mat x3D = mCurrentFrame.UnprojectStereo(i);
                    MapPoint* pNewMP = new MapPoint(x3D, pKF, mpMap);
                    // 这些添加属性的操作是每次创建MapPoint后都要做的
                    pNewMP->AddObservation(pKF, i);
                    pKF->AddMapPoint(pNewMP, i);
                    pNewMP->ComputeDistinctiveDescriptors();
                    mpMap->AddMapPoint(pNewMP);

                    mCurrentFrame.mvpMapPoints[i] = pNewMP;
                    // nPoints++;   // nPoints是记录其中不需要创建地图点的个数，这里应该是不需要的
                } else {
                    // 因为从近到远排序，记录其中不需要创建地图点的个数
                    nPoints++;
                }

                // Step3.4 停止新建地图点必须同时满足以下两个条件：
                // 1. 当前的点的深度已经超过了设定的深度阈值(35倍基线)
                // 2. nPoints已经超过了100个点，说明距离比较远了，可能不准确，停掉退出
                if (vDepthIdx[j].first > mfThDepth && nPoints > 100) {
                    break;
                }

            }
        }
    }

    // Step4 插入关键帧
    // 关键帧插入列表 mlNewKeyFrames中，等待Local Mapping线程处理
    // 将关键帧传给LocalMapping线程
    mpLocalMapper->InsertKeyFrame(pKF);

    // 插入好了，允许局部建图停止
    mpLocalMapper->SetNotStop(false);

    // 当前帧成为新的关键帧，更新
    mnLastKeyFrameId = mCurrentFrame.mnId;
    mpLastKeyFrame = pKF;
}


/**
 * @brief 检查上一帧(mLastFrame)的地图点是否有需要替换的地图点，如果有就进行替换
 * 目的：维护地图点的最新状态，确保上一帧(mLastFrame)中使用的地图点始终是经过优化或融合后的最新版本。
 *      这一步骤对SLAM系统的鲁棒性和精度至关重要，具体原因如下：
 * 1. ‌处理地图点动态更新
 *      地图点替换机制‌：在SLAM运行过程中，为提高地图精度或减少冗余，部分地图点可能被替换（如通过共视图融合或BA优化后的新点替代旧点）。
 *          CheckReplacedInLastFrame()通过遍历上一帧的所有地图点，检查是否存在被替换的版本（pMP->GetReplaced()），
 *          若有则更新为替换后的地图点（pRep）。
 *      避免陈旧数据干扰‌：若不更新，上一帧中保留的旧地图点可能导致后续跟踪或位姿估计时使用错误的观测数据，从而引入误差。
 * 2. ‌保证跟踪连续性
 *      位姿估计的依赖‌：在跟踪线程（Tracking）中，上一帧的地图点用于初始化当前帧的位姿估计（如运动模型或参考关键帧跟踪）。
 *          若地图点未及时更新，可能导致特征匹配失败或位姿优化发散
 *      局部地图跟踪的准确性‌：在TrackLocalMap阶段，局部地图点的有效性直接影响位姿优化结果。更新后的地图点能提供更准确的投影约束，提升位姿估计精度
 * 3. ‌系统效率与一致性
 *      ‌减少冗余计算‌：直接更新地图点而非重新匹配，避免了重复的特征搜索和关联操作，提升计算效率
 *      维护地图一致性‌：确保所有线程（如跟踪、局部建图、闭环检测）使用同一套最新的地图点数据，避免因版本不一致导致的地图冲突 
 */
void Tracking::CheckReplacedInLastFrame()
{
    for (int i = 0; i < mLastFrame.N; i++) {
        MapPoint* pMP = mLastFrame.mvpMapPoints[i];

        if (pMP) {
            MapPoint* pRep = pMP->GetReplaced();
            if (pRep) {
                mLastFrame.mvpMapPoints[i] = pRep;
            }
        }
    }
}


/**
 * @brief 用参考关键帧的地图点来对当前普通帧进行跟踪
 * 
 * Step1 将当前普通帧的描述子转化为BoW向量
 * Step2 通过词袋BoW加速当前帧与参考帧之间的特征点匹配
 * Step3 将上一帧的位姿作为当前帧位姿的初始值
 * Step4 通过优化3D-2D的重投影误差来获得位姿
 * Step5 剔除优化后的匹配点中的外点
 * @return 如果匹配数超10，返回true，否则返回false
 */
bool Tracking::TrackReferenceKeyFrame()
{
    // Compute Bag of Words vector
    // Step1 将当前帧的描述子转化为BoW向量
    mCurrentFrame.ComputeBoW();

    // We perform first an ORB matching with the reference keyframe 我们首先与参考关键帧进行ORB特征匹配。
    // If enough matches are found we setup a PnP solver
    ORBmatcher matcher(0.7, true);
    std::vector<MapPoint*> vpMapPointMatches;

    // Step2 通过词袋BoW加速当前帧与参考帧之间的特征点匹配
    int nmatches = matcher.SearchByBoW(mpReferenceKF,       // 参考关键帧
                                       mCurrentFrame,       // 当前帧
                                       vpMapPointMatches);  // 存储匹配关系
    
    std::cout << "fangguanshou nmatches = " << nmatches << std::endl;

    // 匹配数目小于15，认为跟踪失败
    if (nmatches < 15) {
        return false;
    }

    // Step3 将上一帧的位姿作为当前帧的初始值
    mCurrentFrame.mvpMapPoints = vpMapPointMatches; //
    mCurrentFrame.SetPose(mLastFrame.mTcw);         // 用上一次的Tcw设置初始值，在PosOptimization可以收敛快一些

    // Step4 通过优化3D-2D的重投影误差来获得位姿
    Optimizer::PoseOptimization(&mCurrentFrame);

    // Step5 剔除优化后才剔除外点，是因为在优化过程中就有了对这些外点的标记
    int nmatchesMap = 0;
    for (int i = 0; i < mCurrentFrame.N; i++) {
        if (mCurrentFrame.mvpMapPoints[i]) {    // 存在地图点
            // 如果对应到的某个特征点是外点
            if (mCurrentFrame.mvbOutlier[i]) {
                // 清除它在当前帧中存在过的痕迹
                MapPoint* pMP = mCurrentFrame.mvpMapPoints[i];

                mCurrentFrame.mvpMapPoints[i] = static_cast<MapPoint*>(NULL);
                mCurrentFrame.mvbOutlier[i] = false;
                pMP->mbTrackInView = false;
                pMP->mnLastFrameSeen = mCurrentFrame.mnId;
                nmatches--;
            } else if (mCurrentFrame.mvpMapPoints[i]->Observations() > 0) { // 当前帧的某个地图点的观察帧数量大于0
                // 匹配的内点计数++
                nmatchesMap++;
            }
        }
    }

    std::cout << "fangguanshou nmatchesMap = " << nmatchesMap << std::endl;

    // 跟踪成功的数目超过10才认为跟踪成功，否则跟踪失败
    return nmatchesMap >= 10;
}


/**
 * @brief 根据恒定速度模型用上一帧地图点来对当前帧进行跟踪
 * Step1 更新上一帧的位姿：对于双目或RGBD相机，还会根据深度值生成临时地图点
 * Step2 根据上一帧特征点对应地图点进行投影匹配
 * Step3 优化当前帧位姿
 * Step4 剔除地图点中外点
 * @return 如果匹配数大于10，认为跟踪成功
 */
bool Tracking::TrackWithMotionModel()
{
    // 最小距离 < 0.9*次小距离，匹配成功，检查旋转
    ORBmatcher matcher(0.9, true);

    // Step1 更新上一帧的位姿：对于双目或RGBD相机，还会根据深度值生成临时地图点
    UpdateLastFrame();

    // Step2 根据之前估计的速度，用恒速模型得到当前帧的初始位姿
    mCurrentFrame.SetPose(mVelocity*mLastFrame.mTcw);

    // 清空当前帧的地图点
    std::fill(mCurrentFrame.mvpMapPoints.begin(), mCurrentFrame.mvpMapPoints.end(), static_cast<MapPoint*>(NULL));

    // Project points seen in previous frame
    // 设置特征匹配过程中的搜索半径
    int th;
    if (mSensor != System::STEREO) {
        th = 15;    // 单目、RGBD
    } else {
        th = 7;     // 双目
    }

    // Step3 用上一帧地图点进行投影匹配，如果匹配点不够、则扩大搜索半径再来一次
    int nmatches = matcher.SearchByProjection(mCurrentFrame, mLastFrame, th, mSensor == System::MONOCULAR);

    // 如果匹配点太少，则扩大搜索半径再来一次
    if (nmatches < 20) {
        std::fill(mCurrentFrame.mvpMapPoints.begin(), mCurrentFrame.mvpMapPoints.end(), static_cast<MapPoint*>(NULL));
        nmatches = matcher.SearchByProjection(mCurrentFrame, mLastFrame, 2*th, mSensor == System::MONOCULAR);
    }

    // 如果还是不能获得足够的匹配点，那么就认为跟踪失败
    if (nmatches < 20) {
        return false;
    }

    // Step4 利用3D-2D投影关系，优化当前帧位姿
    Optimizer::PoseOptimization(&mCurrentFrame);

    // Step5 剔除地图点中外点
    int nmatchesMap = 0;
    for (int i = 0; i < mCurrentFrame.N; i++) {
        if (mCurrentFrame.mvpMapPoints[i]) {
            if (mCurrentFrame.mvbOutlier[i]) {
                // 如果优化后判断某个地图点是外点，清除它的所有关系
                MapPoint* pMP = mCurrentFrame.mvpMapPoints[i];

                mCurrentFrame.mvpMapPoints[i] = static_cast<MapPoint*>(NULL);
                mCurrentFrame.mvbOutlier[i] = false;
                pMP->mbTrackInView = false;
                pMP->mnLastFrameSeen = mCurrentFrame.mnId;
                nmatches--;
            } else if (mCurrentFrame.mvpMapPoints[i]->Observations() > 0) {
                // 累加成功匹配到的地图点数目
                nmatchesMap++;
            }
        }
    }

    if (mbOnlyTracking) {
        // 纯定位模式下，如果成功跟踪的地图点非常少，那么这里的mnVO标识就会置位
        mbVO = nmatchesMap < 10;
        return nmatches > 20;
    }

    // Step6 匹配超过10个点就认为跟踪成功
    return nmatchesMap >= 10;
}


/**
 * @brief 重定位过程
 * @return bool
 * 
 * Step1 计算当前帧特征点的词袋向量
 * Step2 找到与当前帧相似的候选关键帧
 * Step3 通过BoW进行匹配
 * Step4 通过EPnP算法估计姿态
 * Step5 通过PoseOptimization对姿态进行优化求解
 * Step6 如果内点较少，则通过投影的方式对之前未匹配的点进行匹配，再进行优化求解
 */
bool Tracking::Relocalization()
{
    // Compute Bag of Words Vector
    // Step1 计算当前帧特征点的词袋向量
    mCurrentFrame.ComputeBoW();

    // Relocalization is performed when tracking is lost    Relocalization()模型是在跟踪陷入LOST状态时执行
    // Track Lost: Query KeyFrame Database for keyframe candidates for relocalisation
    // Step2 找到与当前帧相似的候选关键帧
    std::vector<KeyFrame*> vpCandidateKFs = mpKeyFrameDB->DetectRelocalizationCandidates(&mCurrentFrame);

    // 如果没有候选关键帧，则退出
    if (vpCandidateKFs.empty()) {
        return false;
    }

    const int nKFs = vpCandidateKFs.size();

    // We perform first an ORB matching with each candidate 
    // If enough matches are found we setup a PnP solver.
    ORBmatcher matcher(0.75, true);
    // 每个关键帧的解算器
    std::vector<PnPsolver*> vpPnPsolvers;
    vpPnPsolvers.resize(nKFs);

    // 每个关键帧和当前帧中特征点的匹配关系
    std::vector<std::vector<MapPoint*>> vvpMapPointMatches;
    vvpMapPointMatches.resize(nKFs);

    // 放弃某个关键帧的标记
    std::vector<bool> vbDiscarded;
    vbDiscarded.resize(nKFs);

    // 有效的候选关键帧数目
    int nCandidates = 0;

    // Step3 遍历所有的候选关键帧，通过词袋进行快速匹配，用匹配结果初始化PnP Solver
    // 候选关键帧与当前帧进行匹配，如果匹配点大于15个点就创建求解器
    for (int i = 0; i < nKFs; i++) {
        KeyFrame* pKF = vpCandidateKFs[i];
        if (pKF->isBad()) {
            vbDiscarded[i] = true;
        } else {
            // 当前帧和候选帧用BoW进行快速匹配，匹配结果记录在vvpMapPointMatches，nmatches表示匹配数目
            int nmatches = matcher.SearchByBoW(pKF, mCurrentFrame, vvpMapPointMatches[i]);
            // 如果和当前帧的匹配数小于15，那么就只能放弃这个关键帧
            if (nmatches < 15) {
                vbDiscarded[i] = true;
                continue;
            } else {
                // 如果匹配数目够用，用匹配结果初始化EPnPsolver
                PnPsolver* pSolver = new PnPsolver(mCurrentFrame, vvpMapPointMatches[i]);
                pSolver->SetRansacParameters(0.99,      // 用于计算RANSAC迭代次数理论值的概率
                                             10,        // 最小内点数，但是要注意在程序中实际上是min(给定最小内点数，最小集，内点数理论值)，不一定使用这个
                                             300,       // 最大迭代次数
                                             4,         // 最小集(求解这个问题在一次采样中所需采样的最少的点的个数，对于Sim3是3，EPnP是4)，参与到最小内点数的确定过程中
                                             0.5,       // 这个是表示（最小内点数/样本总数）；实际上RANSAC正常退出的时候所需要的最小内点数其实是根据这个量计算得到的
                                             5.991);    // 自由度为2的卡方检验的阈值，程序中还会根据特征点所在的图层对这个阈值进行缩放
                vpPnPsolvers[i] = pSolver;
                nCandidates++;
            }
        }
    }

    // 是否已经找到相匹配的关键帧的标志
    bool bMatch = false;
    ORBmatcher matcher2(0.9, true);

    // Step4 通过一系列操作，直到找到能够匹配上的关键帧
    // 为什么搞这么复杂？答：是担心误闭环
    while (nCandidates > 0 && !bMatch) {
        // 遍历当前所有的候选关键帧
        for (int i = 0; i < nKFs; i++) {
            // 忽略放弃的
            if (vbDiscarded[i]) {
                continue;
            }

            // Perform 4 Ransac Iterations
            // 内点标记
            std::vector<bool> vbInliers;
            // 内点数
            int nInliers;
            // 表示RANSAC已经没有更多的迭代次数可用 -- 也就是说数据不够好，RANSAC也已经尽力了
            bool bNoMore;

            // Step4.1 通过EPnP算法估计姿态，迭代5次
            PnPsolver* pSolver = vpPnPsolvers[i];
            cv::Mat Tcw = pSolver->iterate(5, bNoMore, vbInliers, nInliers);    // 迭代次数：5次

            // If Ransac reachs max. iterations discard keyframe
            // bNoMore 为 true，表示已经超过了RANSAC最大迭代次数，就放弃当前关键帧
            if (bNoMore) {
                vbDiscarded[i] = true;
                nCandidates--;
            }

            // If a Camera Pose is computed, optimize
            if (!Tcw.empty()) {
                // Step4.2 如果EPnP，计算出了位姿，对内点进行BA优化
                Tcw.copyTo(mCurrentFrame.mTcw);

                // EPnP里RANSAC后的内点的集合
                std::set<MapPoint*> sFound;
                
                const int np = vbInliers.size();
                // 遍历所有内点
                for (int j = 0; j < np; j++) {
                    if (vbInliers[j]) {
                        mCurrentFrame.mvpMapPoints[j] = vvpMapPointMatches[i][j];
                        sFound.insert(vvpMapPointMatches[i][j]);
                    } else {
                        mCurrentFrame.mvpMapPoints[j] = NULL;
                    }
                }
                // 只优化位姿，不优化地图点的坐标，返回的是内点的数量
                int nGood = Optimizer::PoseOptimization(&mCurrentFrame);

                // 如果优化之后的内点数目不多，跳过了当前候选帧，但是却没有放弃当前帧的重定位
                if (nGood < 10) {
                    continue;
                }

                // 删除外点对应的地图点
                for (int io = 0; io < mCurrentFrame.N; io++) {
                    if (mCurrentFrame.mvbOutlier[io]) { // 如果是外点
                        mCurrentFrame.mvpMapPoints[io] = static_cast<MapPoint*>(NULL);
                    }
                }

                // Step4.3 如果内点较少，则通过投影的方式对未匹配的点进行匹配，再进行优化求解
                // 前面的匹配关系是通过词袋匹配过程得到的
                if (nGood < 50) {
                    // 通过投影的方式将关键帧中未匹配的地图点投影到当前帧中，生成新的匹配
                    int nadditional = matcher2.SearchByProjection(mCurrentFrame,        // 当前帧
                                                                  vpCandidateKFs[i],    // 关键帧
                                                                  sFound,               // 已经找到的地图点集合，不会用于PnP
                                                                  10,                   // 窗口阈值，会乘以金字塔尺度
                                                                  100);                 // 匹配的ORB描述子距离应该小于这个阈值
                    // 如果通过投影过程新增了比较多的匹配特征点
                    if (nadditional + nGood >= 50) {
                        // 根据投影匹配的结果，再次采用3D-2D PnP BA优化位姿
                        nGood = Optimizer::PoseOptimization(&mCurrentFrame);

                        // Step4.4 如果BA后内点数还是比较少(<50)但是还不至于太少(>30)，可以挽救下，最后垂死挣扎
                        // 重新执行上一步4.3的过程，只不过使用更小的搜索窗口
                        // 这里的位姿已经使用了更多的点进行了优化，应该更准，所以使用更小的窗口搜索
                        if (nGood > 30 && nGood < 50) {
                            // 用更小窗口、更严格的描述子阈值，重新进行投影搜索匹配
                            sFound.clear();
                            for (int ip = 0; ip < mCurrentFrame.N; ip++) {
                                if (mCurrentFrame.mvpMapPoints[ip]) {
                                    sFound.insert(mCurrentFrame.mvpMapPoints[ip]);
                                }
                            }
                            nadditional = matcher2.SearchByProjection(mCurrentFrame,        // 当前帧
                                                                      vpCandidateKFs[i],    // 候选的关键帧
                                                                      sFound,               // 已经找到的地图点，不会用于PnP
                                                                      3,                    // 新的窗口阈值，会乘以金字塔尺寸
                                                                      64);                  // 匹配的ORB描述子距离应该小于这个阈值
                            // Final optimization
                            // 如果成功挽救回来，匹配数目达到要求，最后BA优化一下
                            if (nGood + nadditional >= 50) {
                                nGood = Optimizer::PoseOptimization(&mCurrentFrame);
                                // 更新地图点
                                for (int io = 0; io < mCurrentFrame.N; io++) {
                                    if (mCurrentFrame.mvbOutlier[io]) {
                                        mCurrentFrame.mvpMapPoints[io] = NULL;
                                    }
                                }
                            }
                            // 如果还是不能够满足就放弃了
                        }
                    }
                }
                
                // If the pose is supported by enough inliers stop ransacs and continue
                // 如果对于当前的候选关键帧已经有足够的内点(>50个)，就退出循环，不考虑其它候选关键帧了
                if (nGood >= 50) {
                    bMatch = true;
                    // 只要有一个候选关键帧重定位成功，就退出循环，不考虑其它候选关键帧了
                    break;
                }
            }
        }   // 一直运行，直到已经没有足够的关键帧，或者是已经有成功匹配上的关键帧
    }

    // 还是没有匹配上
    if (!bMatch) {
        return false;
    } else {
        // 如果匹配上了，说明当前帧重定位成功了（当前帧已经有了自己的位姿）
        mnLastRelocFrameId = mCurrentFrame.mnId;
        return true;
    }
}


/**
 * @brief 用局部地图进行跟踪，进一步优化位姿
 * 
 * 1. 更新局部地图，包括局部关键帧和关键点
 * 2. 对局部MapPoints进行投影匹配
 * 3. 根据匹配对估计当前帧的姿态
 * 4. 根据姿态剔除误匹配
 * @return true if success
 * 
 * Step1 更新局部关键帧mvpLocalKeyFrames和局部地图点mvpLocalMapPoints
 * Step2 在局部地图中查找与当前帧匹配的MapPoints，其实也就是对局部地图点进行跟踪
 * Step3 更新局部所有MapPoints后对位姿再次优化
 * Step4 更新当前帧的MapPoints被观测程度，并统计跟踪局部地图的效果
 * Step5 决定是否跟踪成功
 */
bool Tracking::TrackLocalMap()
{
    // Step1 更新局部关键帧 mvpLocalKeyFrames 和 局部地图点 mvpLocalMapPoints
    UpdateLocalMap();

    // Step2 筛选局部地图中新增的在视野范围内的地图点，投影到当前帧搜索匹配，得到更多的匹配关系
    SearchLocalPoints();

    // Step3 前面新增了更多的匹配关系，BA优化得到更准确的位姿
    Optimizer::PoseOptimization(&mCurrentFrame);
    mnMatchesInliers = 0;

    // Update MapPoint Statistics
    // Step4 更新当前帧的地图点被观测程度，并统计跟踪局部地图点匹配数目
    for (int i = 0; i < mCurrentFrame.N; i++) {
        if (mCurrentFrame.mvpMapPoints[i]) {
            // 由于当前帧的地图点可以被当前帧观测到，其被观测统计量加1
            mCurrentFrame.mvpMapPoints[i]->IncreaseFound();
            if (!mbOnlyTracking) {
                // 如果该地图点被相机观测数目 nObs 大于0， 匹配内点计算 +1
                // nObs: 被观测到的相机数目，单目+1，双目或RGBD则+2
                if (mCurrentFrame.mvpMapPoints[i]->Observations() > 0) {
                    mnMatchesInliers++;
                }
            } else {
                // 记录当前帧跟踪到的地图点数目，用于统计跟踪效果
                mnMatchesInliers++;
            }
        } else if (mSensor == System::STEREO) {
            mCurrentFrame.mvpMapPoints[i] = static_cast<MapPoint*>(NULL);
        }
    }

    // Step5 根据跟踪匹配数目及重定位情况决定是否跟踪成功
    // 如果最近刚刚发生了重定位，那么至少成功匹配50个点才认为是成功跟踪
    if (mCurrentFrame.mnId < mnLastRelocFrameId + mnMaxFrames && mnMatchesInliers < 50) {
        return false;
    }

    if (mnMatchesInliers < 30) {
        return false;
    } else {
        return true;
    }
}


/**
 * @brief 更新上一帧位姿，在上一帧中生成临时地图点
 * 单目情况：只计算上一帧的世界坐标系位姿
 * 双目和RGBD情况：选取有 深度值的并且没有被选为地图点的特征点
 */
void Tracking::UpdateLastFrame()
{
    // Step1 利用参考关键帧更新上一帧在世界坐标系下的位姿
    // 上一普通帧的参考关键帧，注意这里用的是参考关键帧(位姿)而不是上上一帧的普通帧
    KeyFrame* pRef = mLastFrame.mpReferenceKF;
    // ref_key 到 lastframe 的位姿变换
    cv::Mat Tlr = mlRelativeFramePoses.back();

    // 将上一帧的世界坐标系下的位姿计算出来
    // l:last r:reference w:world
    // Tlw = Trl*Trw
    mLastFrame.SetPose(Tlr*pRef->GetPose());

    // 如果上一帧为关键帧，或者单目的情况，则退出，非仅跟踪模式
    if (mnLastKeyFrameId == mLastFrame.mnId || mSensor == System::MONOCULAR || !mbOnlyTracking) {
        return;
    }
    
    // Step2 对于双目或rgbd相机，为上一帧生成新的临时地图点
    // 注意这些地图点只是用来跟踪，不加入到地图中，跟踪完后会删除

    // Step2.1 得到上一帧中具有有效深度值的特征点（不一定是地图点）
    std::vector<std::pair<float, int>> vDepthIdx;
    vDepthIdx.reserve(mLastFrame.N);
    for (int i = 0; i < mLastFrame.N; i++) {
        float z = mLastFrame.mvDepth[i];
        if (z > 0) {    // 要求存在深度值
            vDepthIdx.push_back(std::make_pair(z, i));
        }
    }

    // 如果上一帧中没有有效深度的点，那么就直接退出
    if (vDepthIdx.empty()) {
        return;
    }

    // 按照深度从小到大排序
    std::sort(vDepthIdx.begin(), vDepthIdx.end());

    // Step2.2 从中找出不是地图点的部分
    int nPoints = 0;
    for (size_t j = 0; j < vDepthIdx.size(); j++) {
        int i = vDepthIdx[j].second;
        bool bCreateNew = false;

        // 如果这个点对应在上一帧中的地图点没有，或者创建后就没有被观测到，那么就生成一个临时的地图点
        MapPoint* pMP = mLastFrame.mvpMapPoints[i];
        if (!pMP) {
            bCreateNew = true;
        } else if (pMP->Observations() < 1) {
            // 地图点被创建后就没有被观测，认为不靠谱，也需要重新创建
            bCreateNew = true;
        }

        if (bCreateNew) {
            // Step2.3 需要创建的点，包装为地图点，只是为了提高双目和RGBD的跟踪成功率。并没有添加复杂属性，因为后面会丢弃
            cv::Mat x3D = mLastFrame.UnprojectStereo(i);    // 将mLastFrame帧中第i个特征点转化为世界坐标系下的3维点
            MapPoint* pNewMP = new MapPoint(x3D,            // 世界坐标系坐标
                                            mpMap,          // 跟踪的全局地图
                                            &mLastFrame,    // 存在这个特征点的帧（上一帧）
                                            i);             // 特征点id
            // 加入上一帧的地图点中
            mLastFrame.mvpMapPoints[i] = pNewMP;

            // 标记为临时添加的MapPoint，之后在CreateNewKeyFrame之前会全部删除
            mlpTemporalPoints.push_back(pNewMP);
            // nPoints++;  // nPoints是找出不是地图点的数量，这里是不是没有必要？？？？？？？？？？？？
        } else {
            // 因为从近到远排序，记录其中不选哟创建地图点的个数
            nPoints++;
        }

        // Step2.4 如果地图点质量不好，停止创建地图点
        // 停止新增临时地图点必须同时满足以下条件：
        // 1. 当前的点的深度已经超过了设定的深度阈值（35倍基线）
        // 2. nPoints已经超过了100个点，说明距离比较远了，停掉退出
        if (vDepthIdx[j].first > mfThDepth && nPoints > 100) {
            break;
        }
    }
}


/**
 * @brief 更新LocalMap
 * 局部地图包括：
 * 1. k1个关键帧，k2个临近关键帧和参考关键帧
 * 2. 由这些关键帧观测到的MapPoints
 */
void Tracking::UpdateLocalMap()
{
    // This is for visualization
    // 设置参考地图点用于绘图显示局部地图点(红色)
    mpMap->SetReferenceMapPoints(mvpLocalMapPoints);

    // Update
    // 用共视图来更新关键帧和局部地图点
    UpdateLocalKeyFrames();
    UpdateLocalPoints();

}


/**
 * @brief 跟踪局部地图函数里，更新局部关键帧
 * 方法是遍历当前帧的地图点，将观测到这些地图点的关键帧和相邻的关键帧及其父子关键帧，作为mvpLocalKeyFrames
 * Step1 遍历当前帧的地图点，记录所有能观测到当前帧地图点的关键帧
 * Step2 更新局部关键帧(mvpLocalKeyFrames)，添加局部关键帧包括以下3种类型
 *      类型1：能观测到当前帧地图点的关键帧，也称一级共视关键帧
 *      类型2：一级共视关键帧的共视关键帧，称为二级共视关键帧
 *      类型3：一级共视关键帧的子关键帧、父关键帧
 * Step3 更新当前帧的参考关键帧，与自己共视程度最高的关键帧作为参考关键帧
 */
void Tracking::UpdateLocalKeyFrames()
{
    // Each map point vote for the keyframes in which it has been observed 每个地图点为其被观测到的关键帧投票
    // Step1 遍历当前帧的地图点，记录所有能观测到当前帧地图点的关键
    std::map<KeyFrame*, int> keyframeCounter;
    for (int i = 0; i < mCurrentFrame.N; i++) {
        if (mCurrentFrame.mvpMapPoints[i]) {
            MapPoint* pMP = mCurrentFrame.mvpMapPoints[i];
            if (!pMP->isBad()) {
                // 得到观测到该地图点的关键帧和地图点在关键帧中的索引
                const std::map<KeyFrame*, size_t> observations = pMP->GetObservations();
                // 由于一个地图点可以被多个关键帧观测到，因此对于每一次观测，都对观测到这个地图点的关键帧进行累积投票
                for (std::map<KeyFrame*, size_t>::const_iterator it = observations.begin(), itend = observations.end(); it != itend; it++) {
                    // it->first 是地图点看到的关键帧，同一个关键帧看到的地图点会累加到该关键帧计数
                    // 所以最后keyframeCounter 第1个参数表示某个关键帧，第2个参数表示该关键帧看到了多少当前帧(mCurrentFrame)的地图点，也就是共视程度
                    keyframeCounter[it->first]++;
                }
            } else {
                mCurrentFrame.mvpMapPoints[i] = NULL;
            }
        }
    }

    // 如果当前帧没有共视关键帧，返回
    if (keyframeCounter.empty()) {
        return;
    }

    // 存储具有最多观测次数的关键帧
    int maxCnt = 0;
    KeyFrame* pKFmax = static_cast<KeyFrame*>(NULL);

    // Step2 更新局部关键帧（mvpLocalKeyFrames），添加局部关键帧有3种类型
    // 先清空局部关键帧
    mvpLocalKeyFrames.clear();
    // 先申请3倍内存，不够后面再加
    mvpLocalKeyFrames.reserve(3*keyframeCounter.size());

    // Step2.1 类型1：能观测到当前帧地图点的关键帧作为局部关键帧（一级共视关键帧）
    for (std::map<KeyFrame*, int>::const_iterator it = keyframeCounter.begin(), itEnd = keyframeCounter.end(); it != itEnd; it++) {
        KeyFrame* pKF = it->first;

        // 如果设定为要删除的，跳过
        if (pKF->isBad()) {
            continue;
        }

        // 寻找具有最大观测数目的关键帧
        if (it->second > maxCnt) {
            maxCnt = it->second;
            pKFmax = pKF;
        }

        // 添加到局部关键帧的列表里
        mvpLocalKeyFrames.push_back(it->first);

        // 用该关键帧的成员变量 mnTrackReferenceForFrame 记录当前帧的id
        // 表示它已经是当前帧的局部关键帧了，可以防止重复添加局部关键帧
        pKF->mnTrackReferenceForFrame = mCurrentFrame.mnId;
    }

    // Step2.2 遍历一级共视关键帧，寻找更多的局部关键帧
    for (std::vector<KeyFrame*>::const_iterator itKF = mvpLocalKeyFrames.begin(), itEndKF = mvpLocalKeyFrames.end(); itKF != itEndKF; itKF++) {
        // 处理的局部关键帧不超过80
        if (mvpLocalKeyFrames.size() > 80) {    // 说明如果有超过80个局部关键帧就已经足够了，不需要再添加了
            break;
        }

        KeyFrame* pKF = *itKF;

        // 类型2：一级共视关键帧的共视关键帧(前10个)，称为二级共视关键帧(将邻居的邻居拉拢入伙)
        // 如果共视帧不足10帧，那么就返回所有具有共视关系的关键帧
        const std::vector<KeyFrame*> vNeighs = pKF->GetBestCovisibilityKeyFrames(10);
        // vNeighs 是按照共视程度从大到小排列
        for (std::vector<KeyFrame*>::const_iterator itNeighKF = vNeighs.begin(), itEndNeighKF = vNeighs.end(); itNeighKF != itEndNeighKF; itNeighKF++) {
            KeyFrame* pNeighKF = *itNeighKF;
            if (!pNeighKF->isBad()) {
                // mnTrackReferenceForFrame 防止重复添加局部关键帧
                if (pNeighKF->mnTrackReferenceForFrame != mCurrentFrame.mnId) {
                    mvpLocalKeyFrames.push_back(pNeighKF);
                    pNeighKF->mnTrackReferenceForFrame = mCurrentFrame.mnId;
                    // 找到一个就直接退出for循环，共视程度最高的那个
                    break;
                }
            }
        }

        // 类型3：将一级共视关键帧的子关键帧作为局部关键帧（将邻居的孩子们拉入伙）
        const std::set<KeyFrame*> spChilds = pKF->GetChilds();
        for (std::set<KeyFrame*>::const_iterator sit = spChilds.begin(), send = spChilds.end(); sit != send; sit++) {
            KeyFrame* pChildKF = *sit;
            if (!pChildKF->isBad()) {
                if (pChildKF->mnTrackReferenceForFrame != mCurrentFrame.mnId) {
                    mvpLocalKeyFrames.push_back(pChildKF);
                    pChildKF->mnTrackReferenceForFrame = mCurrentFrame.mnId;
                    // 找到一个子关键帧就跳出循环
                    break;
                }
            }
        }

        // 类型3:将一级共视关键帧的父关键帧加入到局部关键帧中
        KeyFrame* pParent = pKF->GetParent();
        if (pParent) {
            // mnTrackReferenceForFrame 防止重复添加局部关键帧
            if (pParent->mnTrackReferenceForFrame != mCurrentFrame.mnId) {
                mvpLocalKeyFrames.push_back(pParent);
                pParent->mnTrackReferenceForFrame = mCurrentFrame.mnId;
            }
        }
    }

    // Step3 更新当前帧的参考关键帧，与自己共视程度最高的关键帧作为参考关键帧
    if (pKFmax) {
        mpReferenceKF = pKFmax;
        mCurrentFrame.mpReferenceKF = mpReferenceKF;
    }
}


/**
 * @brief 更新局部关键点。先把局部地图清空，然后将局部关键帧的有效地图点添加到局部地图中
 */
void Tracking::UpdateLocalPoints()
{
    // Step1 清空局部地图点
    mvpLocalMapPoints.clear();

    // Step2 遍历局部关键帧 mvpLocalKeyFrames
    for (std::vector<KeyFrame*>::const_iterator itKF = mvpLocalKeyFrames.begin(), itEndKF = mvpLocalKeyFrames.end(); itKF != itEndKF; itKF++) {
        KeyFrame* pKF = *itKF;
        const std::vector<MapPoint*> vpMPs = pKF->GetMapPointMatches(); // 局部关键帧的地图点列表

        // Step3 将局部关键帧的地图点添加到mvpLocalMapPoints
        for (std::vector<MapPoint*>::const_iterator itMP = vpMPs.begin(), itEndMP = vpMPs.end(); itMP != itEndMP; itMP++) {
            MapPoint* pMP = *itMP;
            if (!pMP) {
                continue;
            }

            // 用该地图点的成员变量 mnTrackReferenceForFrame 记录当前帧的id
            // 表示它已经是当前帧的局部地图点了，可以防止重复添加局部地图点
            if (pMP->mnTrackReferenceForFrame == mCurrentFrame.mnId) {
                continue;
            }

            if (!pMP->isBad()) {
                mvpLocalMapPoints.push_back(pMP);
                pMP->mnTrackReferenceForFrame = mCurrentFrame.mnId;
            }
        }
    }
}


/**
 * @brief 用局部地图点进行投影匹配，得到更多的匹配关系
 * 注意：局部地图点已经是当前帧地图点的不需要再投影，只需要将此处的并且在视野范围内的点和当前帧进行投影匹配
 * 
 * 步骤：
 * 1.遍历当前帧的地图点，标记这些地图点不参与之后的投影搜索匹配，因为已经有匹配了
 * 2.判断所有局部地图点中除当前帧地图点外的点，是否在当前帧视野范围内
 * 3.如果需要进行投影匹配的点的数目大于0，就进行投影匹配，增加更多的匹配关系
 */
void Tracking::SearchLocalPoints()
{
    // Step1 遍历当前帧的地图点，标记这些地图点不参与之后的投影搜索匹配
    for (std::vector<MapPoint*>::iterator vit = mCurrentFrame.mvpMapPoints.begin(), vend = mCurrentFrame.mvpMapPoints.end(); vit != vend; vit++) {
        MapPoint* pMP = *vit;
        if (pMP) {
            if (pMP->isBad()) {
                *vit = static_cast<MapPoint*>(NULL);
            } else {
                // 更新能观测到该点的帧数加1（被当前帧观测到了）
                pMP->IncreaseVisible();
                // 标记该点被当前帧观测到，最新的一个关键帧
                pMP->mnLastFrameSeen = mCurrentFrame.mnId;
                // 标记该点在后面的搜索匹配时不被投影，因为已经有匹配了
                pMP->mbTrackInView = false;
            }
        }
    }

    // 准备进行投影匹配的点的数目
    int nToMatch = 0;

    // Step2 判断所有局部地图点中除当前帧地图点外的点，是否在当前帧视野范围内
    for (std::vector<MapPoint*>::iterator vit = mvpLocalMapPoints.begin(), vend = mvpLocalMapPoints.end(); vit != vend; vit++) {
        MapPoint* pMP = *vit;
        // 已经被当前帧观测到的地图点肯定在视野范围内，跳过
        if (pMP->mnLastFrameSeen == mCurrentFrame.mnId) {
            continue;
        }
        // 跳过坏点
        if (pMP->isBad()) {
            continue;
        }
        // 判断地图点是否在当前帧视野内
        if (mCurrentFrame.isInFrustum(pMP, 0.5)) {
            // 观测到该点的帧数加1
            pMP->IncreaseVisible();
            // 只有在视野范围内的地图点才参与之后的投影匹配
            nToMatch++;
        }
    }

    // Step3 如果需要进行投影匹配的点的数目大于0，就进行投影匹配，增加更多的匹配关系
    if (nToMatch > 0) {
        ORBmatcher matcher(0.8);
        int th = 1;
        if (mSensor == System::RGBD) {  // RGBD相机输入的时候，搜索的阈值会变得稍微大一些
                th = 3;
        }
        // 如果不久前进行过重定位,那么就进行一个更加宽泛的搜索,阈值需要增大
        if (mCurrentFrame.mnId < mnLastRelocFrameId + 2) {
            th = 5;
        }
        matcher.SearchByProjection(mCurrentFrame, mvpLocalMapPoints, th);
    }
}




}