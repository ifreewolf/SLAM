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
Frame::Frame(const Frame &frame):
    mpORBvocabulary(frame.mpORBvocabulary),
    mpORBextractorLeft(frame.mpORBextractorLeft),
    mpORBextractorRight(frame.mpORBextractorRight),
    mTimeStamp(frame.mTimeStamp),
    mK(frame.mK.clone()),
    mDistCoef(frame.mDistCoef.clone()),
    mbf(frame.mbf),
    mb(frame.mb),
    mThDepth(frame.mThDepth),
    N(frame.N),
    mvKeys(frame.mvKeys),
    mvKeysRight(frame.mvKeysRight),
    mvKeysUn(frame.mvKeysUn),
    mvuRight(frame.mvuRight),
    mvDepth(frame.mvDepth),
    mBowVec(frame.mBowVec),
    mFeatVec(frame.mFeatVec),
    mDescriptors(frame.mDescriptors.clone()),
    mDescriptorsRight(frame.mDescriptorsRight.clone()),
    mvpMapPoints(frame.mvpMapPoints),
    mvbOutlier(frame.mvbOutlier),
    mnId(frame.mnId),
    mpReferenceKF(frame.mpReferenceKF),
    mnScaleLevels(frame.mnScaleLevels),
    mfScaleFactor(frame.mfScaleFactor),
    mfLogScaleFactor(frame.mfLogScaleFactor),
    mvScaleFactors(frame.mvScaleFactors),
    mvInvScaleFactors(frame.mvInvScaleFactors),
    mvLevelSigma2(frame.mvLevelSigma2),
    mvInvLevelSigma2(frame.mvInvLevelSigma2)
{
    if (!frame.mTcw.empty()) {
        SetPose(frame.mTcw);
    }
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

        // FRAME_GRID_COLS=64，FRAME_GRID_ROWS=48
        // 整张图像的宽度按照64个网格划分，每个网格是多少个像素，再取反
        mfGridElementWidthInv = static_cast<float>(FRAME_GRID_COLS)/static_cast<float>(mnMaxX - mnMinX);
        // 整张图像的宽度按照48个网格划分，每个网格是多少个像素，再取反
        mfGridElementHeightInv = static_cast<float>(FRAME_GRID_ROWS)/static_cast<float>(mnMaxY - mnMinY);

        // 给类的静态成员变量赋值
        fx    = K.at<float>(0, 0);
        fy    = K.at<float>(1, 1);
        cx    = K.at<float>(0, 2);
        cy    = K.at<float>(1, 2);
        // 除法计算需要的时间略长，所以这里直接存储了这个中间计算结果
        invfx = 1.0f / fx;
        invfy = 1.0f / fy;
        // 特殊的初始化过程完成，标志复位
        mbInitialComputations = false;
    }
    // 获取baseline
    mb = mbf / fx;
    // 将特征点分配到图像网格中
    AssignFeaturesToGrid(); // ???????????????????? 分配的目的是什么？？？？？？？？？？？？？
                            // KeyFrame.cc 中 GetFeaturesInArea() 函数根据给定的像素坐标(u,v)和半径radius，确定覆盖的网格，然后快速获取网格内的特征点
                            // ORBmatcher.cc中 SearchByProjection() 函数会调用GetFeaturesInArea() 函数

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
}

void Frame::ComputeStereoFromRGBD(const cv::Mat &imDepth)
{
    mvuRight = std::vector<float>(N, -1);
    mvDepth  = std::vector<float>(N, -1);
    for (int i = 0; i < N; i++) {
        const cv::KeyPoint &kp  = mvKeys[i];    // 矫正前的关键点坐标
        const cv::KeyPoint &kpU = mvKeysUn[i];  // 矫正后的关键点坐标

        const float &u = kp.pt.x;               // 矫正前关键点的x轴坐标
        const float &v = kp.pt.y;               // 矫正前关键点的y轴坐标

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
    if (mDistCoef.at<float>(0) != 0.0) {    // 矫正系数不为0
        cv::Mat mat(4, 2, CV_32F);

        // 整张图片的四个角坐标
        mat.at<float>(0, 0) = 0.0;          // 左上角 x
        mat.at<float>(0, 1) = 0.0;          // 左上角 y

        mat.at<float>(1, 0) = imLeft.cols;  // 右上角 x
        mat.at<float>(1, 1) = 0.0;          // 右上角 y

        mat.at<float>(2, 0) = 0.0;          // 左下角 x
        mat.at<float>(2, 1) = imLeft.rows;  // 左下角 y

        mat.at<float>(3, 0) = imLeft.cols;  // 右下角 x
        mat.at<float>(3, 1) = imLeft.rows;  // 右下角 y

        // Undistort corners
        mat = mat.reshape(2); // 2通道
        cv::undistortPoints(mat, mat, mK, mDistCoef, cv::Mat(), mK);
        mat = mat.reshape(1);   // 恢复1通道

        mnMinX = std::min(mat.at<float>(0, 0), mat.at<float>(2, 0));    // 在左上角x和左下角x之间选择最左边
        mnMaxX = std::min(mat.at<float>(1, 0), mat.at<float>(3, 0));    // 在右上角x和右下角x之间选择最右边
        mnMinY = std::min(mat.at<float>(0, 1), mat.at<float>(1, 1));    // 在左上角y和右上角y之间选择最上边
        mnMaxY = std::min(mat.at<float>(2, 1), mat.at<float>(3, 1));    // 在左下角y和右下角y之间选择最下边

    } else {
        mnMinX = 0.0f;
        mnMaxX = imLeft.cols;
        mnMinY = 0.0f;
        mnMaxY = imLeft.rows;
    }
}


/**
 * @brief 将提取的ORB特征点分配到图像网格中
 */
void Frame::AssignFeaturesToGrid()
{
    // Step1. 给存储特征点的网格数组 Frame::mGrid 预分配空间
    // 0.5的策略是，不可能一个网格超过半数的特征点，因为已经经过四叉树分配了
    int nReserve = 0.5f * N / (FRAME_GRID_COLS * FRAME_GRID_ROWS);

    for (unsigned int i = 0; i < FRAME_GRID_COLS; i++) {
        for (unsigned int j = 0; j < FRAME_GRID_ROWS; j++) {
            mGrid[i][j].reserve(nReserve);
        }
    }

    // Step2 遍历每个特征点，将每个特征点在mvKeysUn中的索引值放到对应的网格mGrid中
    for (int i = 0; i < N; i++) {
        // 从类的成员变量中获取已经去畸变后的特征点
        const cv::KeyPoint &kp = mvKeysUn[i];

        // 存储某个特征点所在网格的网格坐标，如果找到特征点所在的网格坐标，记录在nGridPosX, nGridPosY里，返回true，没找到返回false
        int nGridPosX, nGridPosY;
        // 计算某个特征点所在网格坐标，如果找到特征点所在的网格坐标，记录在mGridPosX, nGridPosY里，返回true，没找到返回false
        if (PosInGrid(kp, nGridPosX, nGridPosY)) {
            // 如果找到特征点所在网格坐标，将这个特征点的索引添加到对应网格的数组mGrid中
            mGrid[nGridPosX][nGridPosY].push_back(i);
        }
    }
}


bool Frame::PosInGrid(const cv::KeyPoint &kp, int &posX, int &posY)
{
    posX = std::round((kp.pt.x - mnMinX) * mfGridElementWidthInv);
    posY = std::round((kp.pt.y - mnMinY) * mfGridElementHeightInv);

    // Keypoint's coordinates are undistorted, which could cause to go out of the image
    if (posX < 0 || posX >= FRAME_GRID_COLS || posY < 0 || posY >= FRAME_GRID_ROWS) {
        return false;
    }

    return true;
}


void Frame::SetPose(cv::Mat Tcw)
{
    mTcw = Tcw.clone();
    UpdatePoseMatrices();
}


void Frame::UpdatePoseMatrices()
{
    mRcw = mTcw.rowRange(0, 3).colRange(0, 3);
    mRwc = mRcw.t();
    mtcw = mTcw.rowRange(0, 3).col(3);
    mOw  = -mRcw.t() * mtcw; // ???????
    // mRcw·mOw + mtcw = 0, 将相机光心在世界坐标系下的坐标，转换到相机坐标系下，是原点[0,0,0,1]^T
    // 求解得：mOw = -mRcw^T·mtcw
}


/**
 * @brief 根据特征点的像素坐标通过内参转变为世界坐标系下的坐标
 */
cv::Mat Frame::UnprojectStereo(const int &i) {
    const float z = mvDepth[i];
    if (z > 0) {
        // u = x*fx/Z + cx
        // v = y*fy/Z + cy
        const float u = mvKeysUn[i].pt.x;
        const float v = mvKeysUn[i].pt.y;
        // x = (u - cx)Z/fx
        // y = (v - cy)Z/fy
        const float x = (u - cx)*z*invfx;
        const float y = (v - cy)*z*invfy;
        cv::Mat x3Dc = (cv::Mat_<float>(3, 1) << x, y, z);  // 相机坐标系下特征点的三维坐标
        return mRwc*x3Dc + mOw; // 返回特征点在世界坐标系的三维坐标
    } else {
        return cv::Mat();
    }
}


/**
 * @brief 计算当前帧特征点对应的词袋BoW，主要是mBowVec 和 mFeatVec
 */
void Frame::ComputeBoW()
{
    // 判断是否以前计算过了，计算过了就跳过
    if (mBowVec.empty()) {
        // 将特征点的描述子转换成词袋向量mBowVec以及特征向量mFeatVec
        std::vector<cv::Mat> vCurrentDesc = Converter::toDescriptorVector(mDescriptors);
        mpORBvocabulary->transform(vCurrentDesc,    // 当前的描述子vector
                                   mBowVec,         // 输出，词袋向量，记录的是单词的id及其对应权重TF-IDF值
                                   mFeatVec,        // 输出，记录node id及其对应的图像feature对应的索引
                                   4);              // 4表示从叶节点向前数的层数
    }

}


/**
 * @brief 找到以x,y为中心，半径为r的圆形内且金字塔层级在[minLevel， maxLevel]的特征点
 * 
 * @param[in] x         特征点半径x
 * @param[in] y         特征点坐标y
 * @param[in] r         搜索半径
 * @param[in] minLevel  最小金字塔层级
 * @param[in] maxLevel  最大金字塔层级
 * @return std::vector<size_t> 返回搜索到的候选匹配点id
 */
std::vector<size_t> Frame::GetFeaturesInArea(const float &x, const float &y, const float &r, const int minLevel, const int maxLevel) const
{
    // 存储搜索结果的vector
    std::vector<size_t> vIndices;
    vIndices.reserve(N);

    // Step1 计算半径为r的圆，上下左右边界所在的网格列和行的id
    // 查找半径为r的圆左侧边界所在网格列坐标
    // mnMinX, mnMaxX, mnMinY, mnMaxY 是图像矫正后获取的结果
    // (mnMaxX - mnMinX)/FRAME_GRID_COLS：表示列方向每个网格可以平均分得几个像素（肯定大于1）
    // mfGridElementWidthInv = FRAME_GRID_COLS / (mnMaxX - mnMinX) 是上面倒数，表示每个像素可以均分几个网格列(肯定小于1)
    // (x - mnMixX - r) 可以看作是从图像的左边界mnMinX到半径r的圆的左边界占的像素列数
    // 两者相乘，就是求出那个半径为r的圆的左侧边界在哪个网格列中
    // 保证nMinCell结果大于等于0
    const int nMinCellX = std::max(0, (int)std::floor((x - mnMinX - r) * mfGridElementWidthInv));
    // 如果最终求得的圆的左边界所在的网格列超过了设定的上限，那么就说明计算出错，找不到符合要求的特征点，返回空vector
    if (nMinCellX >= FRAME_GRID_COLS) { // 64
        return vIndices;
    }

    // 计算圆所在的右边边界网格列索引
    const int nMaxCellX = std::min((int)FRAME_GRID_COLS - 1, (int)std::ceil((x - mnMinX + r) * mfGridElementWidthInv));
    // 如果计算出的圆右边界所在的网格不合法，说明该特征点出错，直接返回空vector
    if (nMaxCellX < 0) {
        return vIndices;
    }

    // 后面的操作类似，计算出这个圆上下界所在的网格行的id
    // 计算圆所在上边界网格行索引
    const int nMinCellY = std::max(0, (int)std::floor((y - mnMinY - r) * mfGridElementHeightInv));
    if (nMinCellY >= FRAME_GRID_ROWS) {
        return vIndices;
    }

    // 计算圆所在下边界网格行索引
    const int nMaxCellY = std::min((int)FRAME_GRID_ROWS - 1, (int)std::ceil((y - mnMinY + r) * mfGridElementHeightInv));
    if (nMaxCellY) {
        return vIndices;
    }

    // 检查需要搜索的图像金字塔层数范围是否符合要求
    const bool bCheckLevels = (minLevel > 0) || (maxLevel >= 0);

    // Step2 遍历圆形区域内的所有网格，寻找满足条件的候选特征点，并将其index放到输出里
    for (int ix = nMinCellX; ix <= nMaxCellX; ix++) {
        for (int iy = nMinCellY; iy <= nMaxCellY; iy++) {
            // 获取这个网格内的所有特征点在 Frame::mvKeysUn 中的索引
            const std::vector<size_t> vCell = mGrid[ix][iy];
            // 如果这个网格中没有特征点，那么就跳过这个网格继续下一个
            if (vCell.empty()) {
                continue;
            }

            // 如果这个网格中特征点，那么就遍历这个图像网格中所有的特征点
            for (size_t j = 0, jend = vCell.size(); j < jend; j++) {
                // 根据索引先读取这个特征点
                const cv::KeyPoint &kpUn = mvKeysUn[vCell[j]];
                // 保证给定的搜索金字塔层级范围合法
                if (bCheckLevels) {
                    // cv::KeyPoint::octave 中表示的是从金字塔的哪一层提取的数据
                    if (kpUn.octave < minLevel) {
                        continue;
                    }
                    if (maxLevel >= 0) {
                        if (kpUn.octave > maxLevel) {
                            continue;
                        }
                    }
                }

                // 通过检查，计算候选特征点到圆中心的距离，查看是否是在这个圆形区域之内
                const float distx = kpUn.pt.x - x;
                const float disty = kpUn.pt.y - y;

                // 如果x方向和y方向的距离都在指定的半径之内，存储其index为候选特征点
                if (std::fabs(distx) < r && fabs(disty) < r) {
                    vIndices.push_back(vCell[j]);
                }
            }
        }
    }
    return vIndices;
}


/**
 * @brief 判断地图点是否在视野内
 * 步骤：
 * Step1 获得这个地图点的世界坐标，经过以下层层关卡的判断，通过的地图点才认为是在视野中
 * Step2 关卡一：将这个地图点变换到当前帧的相机坐标系下，如果深度值为正才能继续下一步
 * Step3 关卡二：将地图点投影到当前帧的像素坐标，如果在图像有效范围内才能继续下一步
 * Step4 关卡三：计算地图点到相机中心的距离，如果在有效距离范围内才能继续下一步
 * Step5 关卡四：计算当前相机指向地图点向量和地图点的平均观测方向夹角，小于60°才能进入下一步
 * Step6 根据地图点到光心的距离来预测一个尺度（仿照特征金字塔层级）
 * Step7 记录计算得到的一些参数
 * 
 * @param[in] pMP               当前地图点
 * @param[in] viewingCosLimit   当前相机指向地图点向量和地图点的平均观测方向夹角阈值
 * @return fasle                地图点不合格，抛弃
 */
bool Frame::isInFrustum(MapPoint* pMP, float viewingCosLimit)
{
    // mbTrackInView 是决定一个地图点是否进行重投影的标志
    // 这个标志的确定要经过多个函数的确定，isInFrustum()只是其中一个验证关卡，这里默认设置为否
    pMP->mbTrackInView = false;

    // 3D in absolute coordinates
    // Step1 获得这个地图点的世界坐标
    cv::Mat P = pMP->GetWorldPos();

    // 3D in camera corrdinates
    // 根据当前帧(粗糙)位姿转化到当前相机坐标系下的三维点Pc
    const cv::Mat Pc = mRcw*P + mtcw;
    const float &PcX = Pc.at<float>(0);
    const float &PcY = Pc.at<float>(1);
    const float &PcZ = Pc.at<float>(2);

    // Check positive depth
    // Step2 关卡一：将这个地图点变换到当前帧的相机坐标系下，如果深度值为正才能继续下一步
    if (PcZ < 0.0f) {
        return false;
    }

    // Project in image and check it is not outside
    // Step3 关卡二：将地图点投影到当前帧的像素坐标，如果在图像有效范围内才能继续下一步
    const float invz = 1.0f / PcZ;
    const float u = fx * PcX * invz + cx;
    const float v = fy * PcY * invz + cy;

    // 判断是否在图像边界中，只要不在，那么就说明无法在当前帧下进行重投影
    if (u < mnMinX || u > mnMaxX) {
        return false;
    }
    if (v < mnMinY || v > mnMaxY) {
        return false;
    }

    // Check distance is in the scale invariance region of the MapPoint
    // Step4 关卡三：计算地图点到相机中心的距离，如果在有效距离范围内才能继续下一步
    // 得到认为的可靠距离：[0.8f*mfMinDistance, 1.2f*mfMaxDistance]
    const float maxDistance = pMP->GetMaxDistanceInvariance();
    const float minDistance = pMP->GetMinDistanceInvariance();

    // 得到当前地图点距离当前帧相机光心的距离，注意P,mOw都是在同一坐标系下才可以
    // mOw:当前相机光心在世界坐标系下的坐标
    const cv::Mat PO = P - mOw;
    // 取模就得到了距离
    const float dist = cv::norm(PO);

    // 如果不在有效范围内，认为投影不可靠
    if (dist < minDistance || dist > maxDistance) {
        return false;
    }

    // Check viewing angle
    // Step5 关卡四：计算当前相机指向地图点向量和地图点的平均观测方向夹角，小于60°才能继续下一步
    cv::Mat Pn = pMP->GetNormal();

    // 计算当前相机指向地图点向量和地图点的平均观测方向夹角的余弦值,注意平均观测方向为单位向量
    const float viewCos = PO.dot(Pn) / dist;

    // 夹角在60°范围内，否则认为观测方向太偏了，重投影不可靠，返回false
    if (viewCos < viewingCosLimit) {
        return false;
    }

    // Predict scale in the image
    // Step6 根据地图点到光心的距离来预测一个尺度（仿照特征点金字塔层级）
    const int nPredictedLevel = pMP->PredictScale(dist, this);  // dist: 这个点到光心的距离，this:输出这个帧

    // Data used by the tracking
    // Step7 记录计算得到的一些参数
    // 通过置位标记 MapPoint::mbTrackInView 来表示这个地图点要被投影
    pMP->mbTrackInView = true;                  // 
    pMP->mTrackProjX = u;                       // 该地图点投影在当前图像（一般是左图）的像素横坐标
    pMP->mTrackProjXR = u - mbf*invz;           // bf/z其实是视差，相减得到右图（如有）中对应点的横坐标
    pMP->mTrackProjY = v;                       // 该地图点投影在当前图像（一般是左图）的像素纵坐标
    pMP->mnTrackScaleLevel = nPredictedLevel;   // 根据地图点到光心距离，预测的地图点的尺度层级
    pMP->mTrackViewCos = viewCos;               // 保存当前相机指向地图点向量和地图点的平均观测方向夹角的余弦值

    // 执行到这里说明这个地图点在相机的视野中并且进行重投影是可靠的，返回true
    return true;

}
}