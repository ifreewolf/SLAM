#include "Frame.h"

namespace ORB_SLAM2
{
long unsigned int Frame::nNextId = 0;
bool Frame::mbInitialComputations = true;
float Frame::fx;
float Frame::fy;
float Frame::cx;
float Frame::cy;
float Frame::invfx;
float Frame::invfy;
float Frame::mnMinX;
float Frame::mnMinY;
float Frame::mnMaxX;
float Frame::mnMaxY;
float Frame::mfGridElementWidthInv;
float Frame::mfGridElementHeightInv;

Frame::Frame()
{

}

// Copy Constructor
Frame::Frame(const Frame &frame)
{

}

// STEREO
Frame::Frame(const cv::Mat &imLeft, const cv::Mat &imRight, const double &timeStamp, ORBextractor* extractorLeft, ORBextractor* extractorRight,
        ORBVocabulary* voc, cv::Mat &K, cv::Mat &distCoef, const float &bf, const float &thDepth)
{

}

// RGBD
Frame::Frame(const cv::Mat &imGray, const cv::Mat &imDepth, const double &timeStamp, ORBextractor* extractor,
        ORBVocabulary* voc, cv::Mat &K, cv::Mat &distCoef, const float &bf, const float &thDepth):
    mpORBvocabulary(voc), mpORBextractorLeft(extractor), mpORBextractorRight(static_cast<ORBextractor*>(NULL)),
    mTimeStamp(timeStamp), mK(K.clone()), mDistCoef(distCoef.clone()), mbf(bf), mThDepth(thDepth)
{
    // Frame ID
    mnId = nNextId++;

    // Scale Level Info
    mnScaleLevels     = mpORBextractorLeft->GetLevels();
    mfScaleFactor     = mpORBextractorLeft->GetScaleFactor();
    mfLogScaleFactor  = std::log(mfScaleFactor);
    mvScaleFactors    = mpORBextractorLeft->GetScaleFactors();
    mvInvScaleFactors = mpORBextractorLeft->GetInverseScaleFactors();
    mvLevelSigma2     = mpORBextractorLeft->GetScaleSigmaSquares();
    mvInvLevelSigma2  = mpORBextractorLeft->GetInverseScaleSigmaSquares();

    // ORB extraction
    ExtractORB(0, imGray);

    N = mvKeys.size();

    if (mvKeys.empty()) {
        return;
    }

    UndistortKeyPoints();

    ComputeStereoFromRGBD(imDepth);

    mvpMapPoints = std::vector<MapPoint*>(N, static_cast<MapPoint*>(NULL));
    mvbOutlier = std::vector<bool>(N, false);

    // This is done only for the first Frame (or after a change in the calibration)
    if (mbInitialComputations) {
        ComputeImageBounds(imGray);

        mfGridElementWidthInv = static_cast<float>(FRAME_GRID_COLS)/static_cast<float>(mnMaxX - mnMinX);
        mfGridElementHeightInv = static_cast<float>(FRAME_GRID_ROWS)/static_cast<float>(mnMaxY - mnMinY);

        fx = K.at<float>(0, 0);
        fy = K.at<float>(1, 1);
        cx = K.at<float>(0, 2);
        cy = K.at<float>(1, 2);
        invfx = 1.0f / fx;
        invfy = 1.0f / fy;

        mbInitialComputations = false;
    }

    mb = mbf / fx;

    AssignFeaturesToGrid();

}

// Monocular
Frame::Frame(const cv::Mat &imGray, const double &timeStamp, ORBextractor* extractor,
        ORBVocabulary* voc, cv::Mat &K, cv::Mat &distCoef, const float &bf, const float &thDepth)
{

}

void Frame::ExtractORB(int flag, const cv::Mat &im)
{
    // 判断是左图还是右图
    if (flag == 0) {
        // 左图的话就使用左图指定的特征点提取器，并将提取结果保存到对应的变量中
        (*mpORBextractorLeft)(im,               // 待提取特征点的图像
                              cv::Mat(),        // 掩膜图像，实际没有用到
                              mvKeys,           // 输出变量，用于保存提取后的特征点
                              mDescriptors);    // 输出变量，用于保存特征点的描述子
    } else {
        // 右图的话就需要使用右图指定的特征点提取器，并将提取结果保存到对应的变量中
        (*mpORBextractorRight)(im, cv::Mat(), mvKeysRight, mDescriptorsRight);
    }
}

/**
 * @brief 用内参对特征点去畸变，结果保存在mvKeysUn中
 */
void Frame::UndistortKeyPoints()
{
    // Step1 如果第一个畸变参数为0，不需要矫正，第一个畸变参数k1是最重要的，一般不为0，为0的话，说明畸变参数都是0
    // 变量mDistCoef中存储了opencv指定格式的去畸变参数，格式为：（k1,k2,p1,p2,k3）
    if (mDistCoef.at<float>(0) == 0.0) {
        mvKeysUn = mvKeys;
        return;
    }

    // Step2 如果畸变参数不为0，用opencv函数进行畸变矫正
    // N为提取的特征点数量，为满足opencv函数输入要求，将N个特征点保存在N*2的矩阵中
    cv::Mat mat(N, 2, CV_32F);
    for (int i = 0; i < N; i++) {
        // 然后将这个特征点的横纵坐标分别保存
        mat.at<float>(i, 0) = mvKeys[i].pt.x;
        mat.at<float>(i, 1) = mvKeys[i].pt.y;
    }

    // Undistort points
    // 特征点矫正
    // 函数 reshape(int cn, int rows=0) 其中cn为更改后的通道数，rows=0表示这个行将保持原来的参数不变
    // 为了能够直接调用opencv的函数来去畸变，需要先将矩阵调整为2通道(对应x,y)
    mat = mat.reshape(2);
    std::cout << "fangguanshou before mK = " << mK << std::endl;
    cv::undistortPoints(mat,        // 输入的特征点坐标
                        mat,        // 输出的矫正后的特征点坐标覆盖原矩阵
                        mK,         // 相机的内参数矩阵
                        mDistCoef,  // 相机畸变参数矩阵
                        cv::Mat(),  // 一个空矩阵，对应为函数原型中的R
                        mK);        // 新内参矩阵，对应为函数原型中的P
    /**
     * 上面函数的mK前后没有变化
     * 
     * P=mK时输出像素坐标
     * 
     * void undistortPoints(
     *  InputArray src,               // 输入畸变点坐标（2xN/Nx2单通道或1xN/Nx1双通道）
     *  OutputArray dst,              // 输出无畸变点坐标
     *  InputArray cameraMatrix,      // 相机内参矩阵3x3
     *  InputArray distCoeffs,        // 畸变系数向量（4/5/8元素）
     *  InputArray R = noArray(),     // 旋转矩阵（双目校正时使用）
     *  InputArray P = noArray()      // 新相机矩阵/投影矩阵
     * );
     * 
     * P矩阵为空时，输出为归一化平面坐标（x,y），需通过内参矩阵转换到像素坐标系；
     * R矩阵用于双目校正场景，单目时可设为空。
     * 计算流程：
     *  1. 归一化坐标：
     *      x'' = (u - cx)/fx, y'' = (v - cy)/fy 归一化平面
     *  2. 去畸变计算：
     *      调用undistort()函数消除径向和切向畸变，得到(x',y')
     *  3. 坐标变换(可选)
     *      若指定R矩阵，进行旋转投影：
     *          [X Y W]^T <-- R·[x' y' 1]^T
     *          最终归一化坐标：x=X/W,y=Y/W
     *  4. 像素坐标转换（若P非空）
     *      u' = x·fx + cx' v' = y·fy' + cy'
     *  
     */
    
    // 调整回只有一个通道，回归我们正常的处理方式
    mat = mat.reshape(1);

    // Step3 存储矫正后的特征点
    mvKeysUn.resize(N);
    // 遍历每一个特征点
    for (int i = 0; i < N; i++) {
        // 根据索引获取这个特征点
        // 注意之所以这样做而不是直接重新声明一个特征点的目的：能够得到源特征点对象的其它属性：
        cv::KeyPoint kp = mvKeys[i];
        // 读取矫正后的坐标并覆盖老坐标
        kp.pt.x = mat.at<float>(i, 0);
        kp.pt.y = mat.at<float>(i, 1);
        mvKeysUn[i] = kp;
    }

    return;
}

void Frame::ComputeStereoFromRGBD(const cv::Mat &imDepth)
{
    mvuRight = std::vector<float>(N, -1);
    mvDepth  = std::vector<float>(N, -1);

    for (int i = 0; i < N; i++) {
        const cv::KeyPoint &kp  = mvKeys[i];    // 矫正前的关键点坐标
        const cv::KeyPoint &kpU = mvKeysUn[i];  // 矫正后的关键点坐标

        const float &v = kp.pt.y;               // 矫正前关键点的y轴坐标
        const float &u = kp.pt.x;               // 矫正前关键点的x轴坐标

        const float d = imDepth.at<float>(v, u);    // 根据矫正前的关键点坐标，去获得特征点对应的深度

        if (d > 0) {
            mvDepth[i]  = d;                // 存储这个关键点的深度
            mvuRight[i] = kpU.pt.x - mbf/d; // RGBD不存在右图，这里是将RGBD模拟成双目相机的左右目图像，这里d是特征点的深度，mbf/d获得的是两个成像的横坐标视差
                                            // kpU.pt.x是模拟成左目图像的x轴坐标，根据视差就可以获得右目图像的x轴坐标，因为用的左图是矫正后的坐标，所以右图的坐标也是矫正过后的
        }
    }
}

void Frame::ComputeImageBounds(const cv::Mat &imLeft)
{
    
}


void Frame::AssignFeaturesToGrid()
{
    
}


}