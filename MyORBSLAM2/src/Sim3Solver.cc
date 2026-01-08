#include "Sim3Solver.h"

namespace ORB_SLAM2
{
Sim3Solver::Sim3Solver(KeyFrame* pKF1, KeyFrame* pKF2, const std::vector<MapPoint*> &vpMatched12, const bool bFixScale):
    mnIterations(0),
    mnBestInliers(0),
    mbFixScale(bFixScale)
{
    mpKF1 = pKF1;
    mpKF2 = pKF2;

    std::vector<MapPoint*> vpKeyFrameMP1 = pKF1->GetMapPointMatches();

    mN1 = vpMatched12.size();

    mvpMapPoints1.reserve(mN1);
    mvpMapPoints2.reserve(mN1);
    mvpMatches12 = vpMatched12;
    mvnIndices1.reserve(mN1);
    mvX3Dc1.reserve(mN1);
    mvX3Dc2.reserve(mN1);

    cv::Mat Rcw1 = pKF1->GetRotation();
    cv::Mat tcw1 = pKF1->GetTranslation();
    cv::Mat Rcw2 = pKF2->GetRotation();
    cv::Mat tcw2 = pKF2->GetTranslation();

    mvAllIndices.reserve(mN1);

    size_t idx = 0;
    for (int i1 = 0; i1 < mN1; i1++) {
        if (vpMatched12[i1]) {
            MapPoint* pMP1 = vpKeyFrameMP1[i1];
            MapPoint* pMP2 = vpMatched12[i1];

            if (!pMP1) {
                continue;
            }

            if (pMP1->isBad() || pMP2->isBad()) {
                continue;
            }

            int indexKF1 = pMP1->GetIndexInKeyFrame(pKF1);
            int indexKF2 = pMP2->GetIndexInKeyFrame(pKF2);

            if (indexKF1 < 0 || indexKF2 < 0) {
                continue;
            }

            const cv::KeyPoint &kp1 = pKF1->mvKeysUn[indexKF1];
            const cv::KeyPoint &kp2 = pKF2->mvKeysUn[indexKF2];

            const float sigmaSquare1 = pKF1->mvLevelSigma2[kp1.octave];
            const float sigmaSquare2 = pKF2->mvLevelSigma2[kp2.octave];

            mvnMaxError1.push_back(9.210*sigmaSquare1);
            mvnMaxError2.push_back(9.210*sigmaSquare2);

            mvpMapPoints1.push_back(pMP1);
            mvpMapPoints2.push_back(pMP2);
            mvnIndices1.push_back(i1);

            cv::Mat X3D1w = pMP1->GetWorldPos();
            mvX3Dc1.push_back(Rcw1*X3D1w + tcw1);   // 地图点在相机中的坐标

            cv::Mat X3D2w = pMP2->GetWorldPos();
            mvX3Dc2.push_back(Rcw2*X3D2w + tcw2);   // 地图点在相机中的坐标

            mvAllIndices.push_back(idx);
            idx++;
        }
    }

    mK1 = pKF1->mK;
    mK2 = pKF2->mK;

    FromCameraToImage(mvX3Dc1, mvP1im1, mK1);
    FromCameraToImage(mvX3Dc2, mvP2im2, mK2);

    SetRansacParameters();

}


void Sim3Solver::SetRansacParameters(double probability, int minInliers, int maxIterations)
{
    mRansacProb = probability;
    mRansacMinInliers = minInliers;
    mRansacMaxIts = maxIterations;

    N = mvpMapPoints1.size();   // number of correspondences

    mvbInliersi.resize(N);

    // Adjust Parameters according to number of correspondences
    // 根据对应点数量调整参数。
    float epsilon = (float)mRansacMinInliers / N;

    // Set RANSAC iterations according to probability, epsilon, and max iterations
    int nIterations;
    
    if (mRansacMinInliers == N) {
        nIterations = 1;
    } else {
        nIterations = std::ceil(std::log(1 - mRansacProb) / std::log(1 - std::pow(epsilon, 3)));
    }

    mRansacMaxIts = std::max(1, std::min(nIterations, mRansacMaxIts));

    mnIterations = 0;
}


cv::Mat Sim3Solver::find(std::vector<bool> &vbInliers12, int &nInliers)
{
    bool bFlag;
    return iterate(mRansacMaxIts, bFlag, vbInliers12, nInliers);
}


/**
 * @brief Ransac求解，mvX3Dc1 和 mvX3Dc2 之间Sim3，函数返回 mvX3Dc2到mvX3Dc1的Sim3变换
 * @param[in] nIterations   设置的最大迭代次数
 * @param[in] bNoMore       为true表示穷尽迭代还没有找到好的结果，说明求解失败
 * @param[in] vbInliers     标记是否是内点
 * @param[in] nInliers      计算得到的Sim3矩阵
 * @return cv::Mat      
 */
cv::Mat Sim3Solver::iterate(int nIterations, bool &bNoMore, std::vector<bool> &vbInliers, int &nInliers)
{
    bNoMore = false;
    vbInliers = std::vector<bool>(mN1, false);
    nInliers = 0;

    // Step1 如果匹配点比要求的最少内点数还少，不满足Sim3求解条件，返回空
    // mRansacMinInliers 表示 RANSAC所需要的最少内点数目
    if (N < mRansacMinInliers) {
        bNoMore = true;
        return cv::Mat();
    }

    // 可以使用的点对的索引，为了避免重复使用
    std::vector<size_t> vAvailableIndices;

    // 随机选择的来自于这两个帧的三对匹配点
    cv::Mat P3Dc1i(3, 3, CV_32F);
    cv::Mat P3Dc2i(3, 3, CV_32F);

    // nCurrentIterations： 当前迭代的次数
    // nIerations:          理论迭代次数
    // mnIerations:         总迭代次数
    // mRansacMaxIts:       最大迭代次数
    int nCurrentIterations = 0;
    // Step2：随机选择三个点，用于求解后面的Sim3
    // 条件1：已经进行的总迭代次数还没有超过限制的最大总迭代次数
    // 条件2：当前迭代次数还没有超过理论迭代次数
    while (mnIterations < mRansacMaxIts && nCurrentIterations < nIterations) {
        nCurrentIterations++;    // 这个函数总迭代的次数
        mnIterations++;         // 总的迭代次数，默认最大迭代次数为300

        vAvailableIndices = mvAllIndices;

        // Step2.1 随机取三组点，取完后从候选索引中删掉
        for (short i = 0; i < 3; ++i) {
            // DBoW3中的随机生成函数
            int randi = DUtils::Random::RandomInt(0, vAvailableIndices.size() - 1);
            int idx = vAvailableIndices[randi];

            // P3Dc1i和P3Dc2i中点的排列顺序：
            // x1 x2 x3
            // y1 y2 y3
            // z1 z2 z3
            mvX3Dc1[idx].copyTo(P3Dc1i.col(i));
            mvX3Dc2[idx].copyTo(P3Dc2i.col(i));

            // 从“可用索引列表”中删除这个点的索引
            vAvailableIndices[randi] = vAvailableIndices.back();
            vAvailableIndices.pop_back();
        }

        // Step2.2 根据随机取的两组匹配的3D点，计算P3Dc2i到P3Dc1i的Sim3变换
        ComputeSim3(P3Dc1i, P3Dc2i);

        // Step2.3 对计算的Sim3变换，通过投影误差进行inlier检测
        CheckInliers();

        // Step2.4 记录并更新最多的内点数目及对应的参数
        if (mnInliersi >= mnBestInliers) {
            mvbBestInliers = mvbInliersi;
            mnBestInliers = mnInliersi;
            mBestT12 = mT12i.clone();
            mBestRotation = mR12i.clone();
            mBestTranslation = mt12i.clone();
            mBestScale = ms12i;

            if (mnInliersi > mRansacMinInliers) {   // 只要计算得到一次合格的Sim变换，就直接返回
                // 返回值，告知得到的内点数目
                nInliers = mnInliersi;
                for (int i = 0; i < N; i++) {
                    if (mvbInliersi[i]) {
                        // 标记为内点
                        vbInliers[mvnIndices1[i]] = true;
                    }
                }
                return mBestT12;
            }   // 如果当前次迭代已经合格了，直接返回
        }   // 更新最多的内点数目
    }   // 迭代循环

    // Step3 如果已经达到了最大迭代次数了还没有得到满足条件的Sim3，说明失败了，放弃，返回
    if (mnIterations >= mRansacMaxIts) {
        bNoMore = true;
    }
    return cv::Mat();   // no more 的时候返回的是一个空矩阵
}


cv::Mat Sim3Solver::GetEstimatedRotation()
{
    return mBestRotation.clone();
}


cv::Mat Sim3Solver::GetEstimatedTranslation()
{
    return mBestTranslation.clone();
}


float Sim3Solver::GetEstimatedScale()
{
    return mBestScale;
}


void Sim3Solver::ComputeCentroid(cv::Mat &P, cv::Mat &Pr, cv::Mat &C)
{
    // P是一个3x3的矩阵，3行表示x,y,z，3列表示3个3D点
    // C是P矩阵的质心
    // Pr是减去质心后的3D点

    // void cv::reduce(InputArray src, OutputArray dst, int dim, int rtype, int dtype = -1)
    // src‌：输入矩阵（二维数组）
    // dst‌：输出的单行/列向量
    // dim‌：降维方向
    // 0：压缩为单行（处理列）；1：压缩为单列（处理行）；-1：自动选择
    // rtype‌：简化操作类型 CV_REDUCE_SUM - 求和；CV_REDUCE_AVG - 求平均值；CV_REDUCE_MAX - 求最大值；CV_REDUCE_MIN - 求最小值 
    cv::reduce(P, C, 1, cv::REDUCE_SUM); // 这里是对P矩阵按行求和，结果存储在C，为3行1列的向量
    C = C / P.cols; //  C是按行求和，这里除以总列数，就是平均值，也就是质心

    for (int i = 0; i < P.cols; i++) {
        Pr.col(i) = P.col(i) - C;   // P矩阵减去质心
    }
}


/**
 * @brief 根据两组匹配的3D点，计算P2到P1的Sim3变换
 * @param[in] P1    匹配的3D点(三个，每个的坐标都是列向量形式，三个点组成了3x3的矩阵)（当前关键帧）
 * @param[in] P2    匹配的3D点(闭环关键帧)
 */
void Sim3Solver::ComputeSim3(cv::Mat &P1, cv::Mat &P2)
{
    // Step1：Centroid and relative coordinates
    // 定义3D点质心及去质心后的点
    // O1和O2分别为P1和P2矩阵中3D点的质心
    // Pr1和Pr2为减去质心后的3D点
    cv::Mat Pr1(P1.size(), P1.type());  // Relative coordinates to centroid (set 1)
    cv::Mat Pr2(P2.size(), P2.type());  // Relative coordinates to centroid (set 2)
    cv::Mat O1(3, 1, Pr1.type());       // Centroid of P1
    cv::Mat O2(3, 1, Pr2.type());       // Centroid of P2

    ComputeCentroid(P1, Pr1, O1);
    ComputeCentroid(P2, Pr2, O2);

    // Step2 Compute M matrix
    // 计算论文中三位点数目 n>3 的M矩阵，这里只使用了3个点
    // Pr2对应论文中 r_l, i', Pr1对应论文中 r_r, i'计算的是P2到P1的Sim3，论文中是left到right的Sim3
    // M = \sum_i^n P'_iQ'_i
    cv::Mat M = Pr2 * Pr1.t();

    // Step3 Compute N matrix
    double N11, N12, N13, N14, N22, N23, N24, N33, N34, N44;

    cv::Mat N(4, 4, P1.type());

    N11 = M.at<float>(0, 0) + M.at<float>(1, 1) + M.at<float>(2, 2);    // Sxx + Syy + Szz
    N12 = M.at<float>(1, 2) - M.at<float>(2, 1);                        // Syz - Szy
    N13 = M.at<float>(2, 0) - M.at<float>(0, 2);                        // Szx - Sxz
    N14 = M.at<float>(0, 1) - M.at<float>(1, 0);                        // Sxy - Syx
    N22 = M.at<float>(0, 0) - M.at<float>(1, 1) - M.at<float>(2, 2);    // Sxx - Syy - Szz
    N23 = M.at<float>(0, 1) - M.at<float>(1, 0);                        // Sxy + Syx
    N24 = M.at<float>(2, 0) + M.at<float>(0, 2);                        // Szx + Sxz
    N33 = -M.at<float>(0, 0) + M.at<float>(1, 1) - M.at<float>(2, 2);   // -Sxx + Syy - Szz
    N34 = M.at<float>(1, 2) + M.at<float>(2, 1);                        // Syz + Szy
    N44 = -M.at<float>(0, 0) - M.at<float>(1, 1) + M.at<float>(2, 2);   // -Sxx - Syy + Szz

    N = (cv::Mat_<float>(4, 4) << N11, N12, N13, N14,
                                  N12, N22, N23, N24,
                                  N13, N23, N33, N34,
                                  N14, N24, N34, N44);
    
    // Step4 特征值分解求最大特征值对应的特征向量，就是我们要求的旋转四元数
    cv::Mat eval, evec; // val, vec
    // 特征值默认是从大到小排列，所以eval[0]是最大值
    cv::eigen(N, eval, evec);   // evec[0] is the quaternion of the desired rotation

    // N矩阵最大特征值（第一个特征值）对应特征向量就是要求的四元数（q0, q1, q2, q3），其中q0是实部
    // 将(q1 q2 q3)放入vec（四元数的虚部）
    cv::Mat vec(1, 3, evec.type());
    (evec.row(0).colRange(1, 4)).copyTo(vec);   // extract imaginary part of the quaternion (sin * axis)

    // 四元数虚部模长 norm(vec) = sin(theta / 2)，四元数实部 evec.at<float>(0, 0) = q0 = cos(theta / 2)
    // 这一步的ang实际是theta/2,theta是旋转向量中旋转角度
    // ？这里也可以用 arccos(q0) = angle / 2得到旋转角度
    // tan(theta/2) = norm(vec)/q0
    double ang = atan2(norm(vec), evec.at<float>(0, 0));

    // vec/norm(vec)归一化得到归一化后的旋转向量，然后乘上角度得到包含了旋转轴和旋转角信息的旋转向量vec
    vec = 2 * ang * vec / norm(vec);   // Angle-axis representation. quaternion angle is the half

    mR12i.create(3, 3, P1.type());
    // 旋转向量（轴角）转换为旋转矩阵
    cv::Rodrigues(vec, mR12i);  // compute the rotation matrix from angle-axis

    // Step5 Rotation set 2
    // 利用刚计算出来的旋转将三维点旋转到同一个坐标系，P3对应论文里的 r_l, i', Pr1对应论文里的r_r, i'
    cv::Mat P3 = mR12i * Pr2;

    // Step6 计算尺度因子 Scale
    if (!mbFixScale) {
        // 论文中有2个求尺度方法，一个是非对称性，一个是对称性
        // 代码里实际使用的是另一种方法，这个公式对应着论文中非对称方式
        // Pr1对应论文里的r_r, i', P3对应论文里的r_l, i', （经过坐标系转换的Pr2）, n=3剩下的就和论文中都一样了
        double nom = Pr1.dot(P3);
        // 准备计算分量
        cv::Mat aux_P3(P3.size(), P3.type());
        aux_P3 = P3;    // 这里就显得有点多余了
        // 先得平方
        // cv::pow 是 OpenCV 库中用于对数组（如图像矩阵）的每个元素进行幂运算的函数
        // 参数说明‌
        //  - src‌：输入数组（图像矩阵）
        //  - power‌：幂指数
        //  - dst‌：输出数组，与 src 大小和类型相同
        // 功能特点‌
        //  - 对输入数组的‌每个元素‌分别计算其 power 次幂
        cv::pow(P3, 2, aux_P3);
        double den = 0;

        // 然后再累加
        for (int i = 0; i < aux_P3.rows; i++) {
            for (int j = 0; j < aux_P3.cols; j++) {
                den += aux_P3.at<float>(i, j);
            }
        }
        // nom‌：计算的是两个坐标系中匹配点集去中心化后的坐标的模长乘积之和
        // den‌：计算的是源坐标系中点集去中心化后的坐标的模长平方之和
        ms12i = nom / den;
    } else {
        ms12i = 1.0f;
    }

    // Step7 计算平移Translation
    mt12i.create(1, 3, P1.type());
    // 论文中平移公式
    mt12i = O1 - ms12i * mR12i * O2;

    // Step8 计算双向变换矩阵，目的是后面得检查的过程中能够进行双向的投影操作
    // Step8.1 用尺度、旋转、平移构建变换矩阵 T12
    mT12i = cv::Mat::eye(4, 4, P1.type());

    cv::Mat sR = ms12i * mR12i;

    sR.copyTo(mT12i.rowRange(0, 3).colRange(0, 3));
    mt12i.copyTo(mT12i.rowRange(0, 3).col(3));

    // Step8.2 T21
    //     [R   t]
    // T = [0   1]
    //        [R^T -R^T.t]
    // T^-1 = [0       1 ]
    // 上面是欧式变换的逆变换,对于相似变换有类似的公式
    //     [sR t]
    // T = [0  1]
    //        [1/sR^T -1/sR^T*t]
    // T^-1 = [0           1   ]
    mT21i = cv::Mat::eye(4, 4, P1.type());

    cv::Mat sRinv = (1.0 / ms12i) * mR12i.t();

    sRinv.copyTo(mT21i.rowRange(0, 3).colRange(0, 3));
    cv::Mat tinv = -sRinv * mt12i;
    tinv.copyTo(mT21i.rowRange(0, 3).col(3));

}


void Sim3Solver::CheckInliers()
{

}


void Sim3Solver::Project(const std::vector<cv::Mat> &vP3Dw, std::vector<cv::Mat> &vP2D, cv::Mat Tcw, cv::Mat K)
{

}


void Sim3Solver::FromCameraToImage(const std::vector<cv::Mat> &vP3Dc, std::vector<cv::Mat> &vP2D, cv::Mat K)
{
    const float &fx = K.at<float>(0, 0);
    const float &fy = K.at<float>(1, 1);
    const float &cx = K.at<float>(0, 2);
    const float &cy = K.at<float>(1, 2);

    vP2D.clear();
    vP2D.reserve(vP3Dc.size());

    for (size_t i = 0, iend = vP3Dc.size(); i < iend; i++) {
        const float invz = 1 / (vP3Dc[i].at<float>(2));
        const float x = vP3Dc[i].at<float>(0)*invz;
        const float y = vP3Dc[i].at<float>(1)*invz;

        vP2D.push_back((cv::Mat_<float>(2, 1) << fx*x + cx, fy*y + cy));
    }
}


}