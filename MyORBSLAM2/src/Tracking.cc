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
                        mpInitializer(static_cast<Initializer*>(NULL))

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
        mfThDepth = mbf*(float)fSettings["ThDepth"]/fx;
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
    std::cout << "System Reseting" << std::endl;
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
    // std::cout << "fangguanshou Track" << std::endl;
}

}