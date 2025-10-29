#include "PnPsolver.h"

namespace ORB_SLAM2
{
// 在大体的pipeline和Sim3Solver差不多，都是 构造->设置RANSAC参数->外部调用迭代函数，进行计算->得到计算的结果
// pcs表示3D点在camera坐标系下的坐标 point of camera
// pws表示3D点在世界坐标系下的坐标 point of world
// us表示图像坐标系下的2D点坐标
// alpha为真实3D点用4个虚拟控制点表示时的系数
// 构造函数
PnPsolver::PnPsolver(const Frame &F, const std::vector<MapPoint*> &vpMapPointMatches):
    pws(0),
    us(0),
    alphas(0),
    pcs(0), // 这里的四个变量都是指针，直接这样写的原因可以参考函数 set_maximum_number_of_correspondences()
    maximum_number_of_correspondences(0),
    number_of_correspondences(0),
    mnInliersi(0),
    mnIterations(0),
    mnBestInliers(0),
    N(0)
{
    // 根据点数初始化容器的大小
    mvpMapPointMatches = vpMapPointMatches;             // 匹配关系
    mvP2D.reserve(F.mvpMapPoints.size());               // 2D特征点
    mvSigma2.reserve(F.mvpMapPoints.size());            // 特征点金字塔层级
    mvP3Dw.reserve(F.mvpMapPoints.size());              // 世界坐标系下的3D点
    mvKeyPointIndices.reserve(F.mvpMapPoints.size());   // 记录被使用特征点在原始特征点容器中的索引，因为有些3D点不一定存在，所以索引是不连续的
    mvAllIndices.reserve(F.mvpMapPoints.size());        // 记录被使用特征点的索引，是连续的

    // 生成地图点、对应2D特征点，记录一些索引坐标
    int idx = 0;
    // 遍历给出的每一个地图点
    for (size_t i = 0, iend = vpMapPointMatches.size(); i < iend; i++) {
        MapPoint* pMP = vpMapPointMatches[i];   // 依次获取每个地图点

        if (pMP) {
            if (!pMP->isBad()) {
                const cv::KeyPoint &kp = F.mvKeysUn[i]; // 得到2维特征点(矫正过的)，将KeyPoint类型变为Point2f

                mvP2D.push_back(kp.pt); // 存放2维特征点
                mvSigma2.push_back(F.mvLevelSigma2[kp.octave]); // 记录特征点是在哪一层提取出来的

                cv::Mat Pos = pMP->GetWorldPos();   // 地图点世界坐标系下的3D点坐标
                mvP3Dw.push_back(cv::Point3f(Pos.at<float>(0), Pos.at<float>(1), Pos.at<float>(2)));

                mvKeyPointIndices.push_back(i); // 记录被使用特征点在原始特征点容器中的索引，mvKeyPointIndices是跳跃的
                mvAllIndices.push_back(idx);    // 记录被使用特征点的索引，mvAllIndices是连续的

                idx++;
            }
        }
    }   // 遍历给出的每一个地图点

    // Set camera calibration parameters
    fu = F.fx;
    fv = F.fy;
    uc = F.cx;
    vc = F.cy;

    // 设置默认的RANSAC参数，这个和Sim3Solver中的操作是相同的
    SetRansacParameters();
}


PnPsolver::~PnPsolver()
{
    delete [] pws;
    delete [] us;
    delete [] alphas;
    delete [] pcs;
}


/**
 * @brief 设置RANSAC迭代的参数
 * @param[in] probability   用于计算RANSAC理论迭代次数所用的比例
 * @param[in] minInliers    退出RANSAC所需要的最小内点个数，注意这个只是给定值，最终迭代的时候不一定按照这个来
 * @param[in] maxIterations 设定的最大RANSAC迭代次数
 * @param[in] minSet        表示求解这个问题所需要的最小的样本数目，简称最小集；参与到最小内点数的确定过程中，默认是4
 * @param[in] epsilon       希望得到的 内点数/总体数 的比值，参与到最小内点数的确定过程中
 * @param[in] th2           内外点判定时的距离的baseline(程序中还会根据特征点所在的图层对这个阈值进行缩放的)
 */
void PnPsolver::SetRansacParameters(double probability,
                                    int minInliers,
                                    int maxIterations,
                                    int minSet,
                                    float epsilon,
                                    float th2)  // 5.991
{
    // 注意这里在每次采样的过程中，需要采样四个点，即最小集应该设置为4

    // Step1 获取给定的参数
    mRansacProb = probability;      // 0.99，用于计算RANSAC迭代次数理论值的比例
    mRansacMinInliers = minInliers; // 10，最小内点数
    mRansacMaxIts = maxIterations;  // 300，最大迭代次数
    mRansacEpsilon = epsilon;       // 0.5，最小内点数/样本总数
    mRansacMinSet = minSet;         // 最小集

    // Step2 计算理论内点数，并且选 max()
    N = mvP2D.size();   // number of correspondences，所有二维特征点个数

    mvbInliersi.resize(N);  // inlier index, mvInliersi记录每次迭代inlier的点个数

    int nMinInliers = N * mRansacEpsilon;
    if (nMinInliers < mRansacMinInliers) {
        nMinInliers = mRansacMinInliers;
    }

    if (nMinInliers < minSet) {
        nMinInliers = minSet;
    }
    mRansacMinInliers = nMinInliers;

    // Step3 根据敲定的“最小内点数”来调整，内点数/总体数 这个比例 epsilon
    // 这个变量确实是希望取得高一些，也可以理解为想让调整之后得内点数 mRansacMinInliers 保持一致。
    if (mRansacEpsilon < (float)mRansacMinInliers / N) {
        mRansacEpsilon = (float)mRansacMinInliers / N;
    }

    int nIterations;
    if (mRansacMinInliers == N) {
        nIterations = 1;
    } else {
        nIterations = std::ceil(std::log(1 - mRansacProb)/std::log(1 - std::pow(mRansacEpsilon, 3)));
    }

    mRansacMaxIts = std::max(1, std::min(nIterations, mRansacMaxIts));

    // Step5 计算不同图层上的特征点在进行内点检验的时候，所使用的不同判断误差阈值
    mvMaxError.resize(mvSigma2.size());
    for (size_t i = 0; i < mvSigma2.size(); i++) {
        mvMaxError[i] = mvSigma2[i] * th2;
    }
}


cv::Mat PnPsolver::find(std::vector<bool> &vbInliers, int &nInliers)
{
    bool bFlag;
    return iterate(mRansacMaxIts, bFlag, vbInliers, nInliers);
}


/**
 * @brief EPnP迭代计算
 * 
 * @param[in] nIterations   迭代次数
 * @param[in] bNoMore       达到最大迭代次数的标志
 * @param[in] vbInliers     内点的标记
 * @param[in] nInliers      总共内点数
 * @return cv::Mat          计算出来的位姿
 */
cv::Mat PnPsolver::iterate(int nIterations, bool &bNoMore, std::vector<bool> &vbInliers, int &nInliers)
{
    bNoMore = false;    // 已经达到最大迭代次数的标志
    vbInliers.clear();
    nInliers = 0;       // 当前迭代时的内点数

    // mRansacMinSet 为每次RANSAC需要的特征点数，默认为4组3D-2D对应点
    set_maximum_number_of_correspondences(mRansacMinSet);   // mRansacMinSet = 4

    // 如果已有匹配点数目比要求的内点数目还少，直接退出
    // N 为所有2D点的个数，mRansacMinInliers 为正常退出RANSAC迭代过程中最少的inlier数
    if (N < mRansacMinInliers) {    // mRansacMinInliers通常为10
        bNoMore = true;
        return cv::Mat();
    }

    // mvAllIndices 为所有参与PnP的2D点的索引
    // vAvailableIndices 为每次从mvAllIndices中随机挑选mRansacMinSet组3D-2D对应点进行一次RANSAC
    std::vector<size_t> vAvailableIndices;

    // 当前的迭代次数id
    int nCurrentIterations = 0;

    // 进行迭代的条件
    // 条件1：历史进行的迭代次数少于最大迭代值
    // 条件2：当前进行的迭代次数少于当前函数给定的最大迭代值
    while (mnIterations < mRansacMaxIts || nCurrentIterations < nIterations) {  // nIterations = 5
        // 迭代次数更新
        nCurrentIterations++;
        mnIterations++;
        // 清空已有的匹配点的计数，为新的一次迭代作准备
        reset_correspondences();

        vAvailableIndices = mvAllIndices;

        // Get min set of points
        // 随机选取4组（默认数目）最小集合
        for (short i = 0; i < mRansacMinSet; ++i) {
            int randi = DUtils::Random::RandomInt(0, vAvailableIndices.size() - 1);

            // 将生成的这个索引映射到给定帧的特征点id
            int idx = vAvailableIndices[randi];

            // 将对应的3D-2D压入到pws和us，这个过程中需要知道将这些点的信息存储到数组中的哪个位置，这个就由变量 number_of_correspondences 来指示
            add_correspondence(mvP3Dw[idx].x, mvP3Dw[idx].y, mvP3Dw[idx].z, mvP2D[idx].x, mvP2D[idx].y);

            // 从“可用索引标”中删除这个已经被使用的点
            vAvailableIndices[randi] = vAvailableIndices.back();
            vAvailableIndices.pop_back();
        }

        // Compute camera pose
        // 计算相机的位姿
        compute_pose(mRi, mti);

        // Check inliers
        // 通过之前求解的位姿来进行3D-2D投影，统计内点数目
        CheckInliers();

        // 如果当前次迭代得到的内点数目已经达到了合格的要求了
        if (mnInliersi >= mRansacMinInliers) {
            // If it is the best solution so far, save it
            // 更新最佳的计算结果
            if (mnInliersi > mnBestInliers) {
                mvbBestInliers  =mvbInliersi;
                mnBestInliers = mnInliersi;

                cv::Mat Rcw(3, 3, CV_64F, mRi);
                cv::Mat tcw(3, 1, CV_64F, mti);
                Rcw.convertTo(Rcw, CV_32F);
                tcw.convertTo(tcw, CV_32F);
                mBestTcw = cv::Mat::eye(4, 4, CV_32F);
                Rcw.copyTo(mBestTcw.rowRange(0, 3).colRange(0, 3));
                tcw.copyTo(mBestTcw.rowRange(0, 3).col(3));
            }

            // 还要求精度
            if (Refine()) { // 如果求精成功（即表示求精之后的结果能够满足退出RANSAC的内点数条件了）
                nInliers = mnRefinedInliers;
                // 转录，作为计算结果
                vbInliers = std::vector<bool>(mvpMapPointMatches.size(), false);
                for (int i = 0; i < N; i++) {
                    if (mvbRefinedInliers[i]) {
                        vbInliers[mvKeyPointIndices[i]] = true;
                    }
                }
                // 直接返回了求精之后的相机位姿
                return mRefinedTcw.clone();
            }

            // 如果求精之后还是达不到能够RANSAC的结果，那么就继续进行RANSAC迭代了
        }
    }

    // 如果执行到这里，说明可能已经超过了上面的两种迭代次数中的一个了，且精度未达要求
    // 如果超过了程序中给定的最大迭代次数
    if (mnIterations >= mRansacMaxIts) {
        // 没有更多的允许迭代次数了
        bNoMore = true;
        // 但是如果我们目前得到的最好结果看上去还不错的话
        if (mnBestInliers >= mRansacMinInliers) {
            // 返回计算结果
            nInliers = mnBestInliers;
            vbInliers = std::vector<bool>(mvpMapPointMatches.size(), false);
            for (int i = 0; i < N; i++) {
                if (mvbBestInliers[i]) {
                    vbInliers[mvKeyPointIndices[i]] = true;
                }
            }
            return mBestTcw.clone();
        }
    }

    // 如果也没有好的计算结果，只好说明迭代失败
    return cv::Mat();
}


void PnPsolver::CheckInliers()
{
    return;
}


bool PnPsolver::Refine()
{
    return false;
}


double PnPsolver::compute_pose(double R[3][3], double T[3])
{
    return 0.0f;
}


void PnPsolver::add_correspondence(const double X, const double Y, const double Z, const double u, const double v)
{
    return;
}


void PnPsolver::set_maximum_number_of_correspondences(const int n)
{
    return;
}


void PnPsolver::reset_correspondences(void)
{
    return;
}
}