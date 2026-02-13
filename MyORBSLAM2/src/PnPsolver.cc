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
        reset_correspondences();    // 就是让number_of_correspondences = 0

        vAvailableIndices = mvAllIndices;

        // Get min set of points
        // 随机选取4组（默认数目）最小集合
        for (short i = 0; i < mRansacMinSet; ++i) {
            int randi = DUtils::Random::RandomInt(0, vAvailableIndices.size() - 1);

            // 将生成的这个索引映射到给定帧的特征点id
            int idx = vAvailableIndices[randi];

            // 将对应的3D-2D压入到pws和us，这个过程中需要知道将这些点的信息存储到数组中的哪个位置，这个就由变量 number_of_correspondences 来指示
            add_correspondence(mvP3Dw[idx].x, mvP3Dw[idx].y, mvP3Dw[idx].z, mvP2D[idx].x, mvP2D[idx].y);    // 

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


/**
 * @brief 使用PnP算法计算相机的位姿，其中匹配点的信息由类的成员函数给定
 * @param[out] R 求解位姿里的旋转矩阵
 * @param[out] T 求解位姿里得平移向量
 * @return double 使用这对旋转和平移位姿的时候，匹配点对的平均重投影误差
 */
double PnPsolver::compute_pose(double R[3][3], double T[3])
{
    // Step1 获得EPnP算法中的四个控制点 c_j^w
    choose_control_points();

    // Step2 计算世界坐标系下每个3D点用4个控制点线性表达时的系数alpha
    compute_barycentric_coordinates();

    // P_i^w = \sum_j=1^n alpha_ij * C_j^w

    // Step3 构造M矩阵，EPnP原始论文中共视(3)(4) --> (5)(6)(7)；矩阵的大小为 2n*12，n为使用的匹配点的对数
    cv::Mat M(2*number_of_correspondences, 12, CV_64F);

    // 根据每一对匹配点的数据来填充矩阵M中的数据
    // alphas:  世界坐标系下3D点用4个虚拟控制点表达时的系数
    // us:      图像坐标系下的2D坐标
    // fill_M
    // 填充M矩阵
    for (int i = 0; i < number_of_correspondences; i++) {
        fill_M(M, 2*i, alphas + 4*i, us[2*i], us[2*i + 1]);
    }

    double mtm[12*12];
    double d[12];
    double utp[12*12];
    cv::Mat MtM(12, 12, CV_64F, mtm);
    cv::Mat D(12, 1, CV_64F, d);        // 特征值
    cv::Mat Ut(12, 12, CV_64F, ut);     // 特征向量

    // Step4.1 求解Mx = 0
    // 求M^TM，为什么求M^TM，因为M=UΣV^T，Mx=0的解空间由矩阵M的右奇异向量（对应零奇异值）张成‌
    cv::gemm(M, M, 1.0, cv::Mat(), 0.0, MtM, cv::GEMM_1_T); // A^T*A
    // 该函数实际是特征值分解，得到特征值D，特征向量ut，对应EPnP论文中式(8)中的vi
    // 最小特征值对应的特征向量就是矩阵方程的最优解，其解就是相机坐标系下的控制点，对应于源码中的Ut
    // cv::SVD::MODIFY_A表示允许修改矩阵A
    cv::SVD::compute(MtM, D, Ut, 0, cv::SVD::MODIFY_A | cv::SVD::U_T);
    // cv::SVD svd(MtM, cv::SVD::MODIFY_A);
    // D = svd.w;
    // Ut = svd.u;

    // Step4.2 计算分情况讨论的时候需要用到的矩阵L和ρ
    // EPnP论文式13中的L和ρ：Lβ=ρ
    double l_6x10[6*10];
    double rho[6];
    cv::Mat L_6x10(6, 10, CV_64F, l_6x10);
    cv::Mat Rho(6, 1, CV_64F, rho);

    // 计算这两个量，6x10是先准备按照EPnP论文中的N=4来计算的，Lβ=ρ
    compute_L_6x10(ut, l_6x10);
    compute_rho(rho);

    double Betas[4][4];
    double rep_errors[4];
    double Rs[4][3][3];
    double ts[4][3];

    // 求解近似解：N=4的情况
    find_betas_approx_1(&L_6x10, &Rho, Betas[1]);
    // 高斯牛顿法迭代优化得到 beta
    gauss_newton(&L_6x10, &Rho, Betas[1]);
    rep_errors[1] = compute_R_and_t(ut, Betas[1], Rs[1], ts[1]);

    // 求解近似解：N=2的情况
    find_betas_approx_2(&L_6x10, &Rho, Betas[2]);
    gauss_newton(&L_6x10, &Rho, Betas[1]);
    rep_errors[2] = compute_R_and_t(ut, Betas[2], Rs[2], ts[2]);

    // 求近似解：N=3的情况
    find_betas_approx_3(&L_6x10, &Rho, Betas[3]);
    gauss_newton(&L_6x10, &Rho, Betas[3]);
    rep_errors[3] = compute_R_and_t(ut, Betas[3], Rs[3], ts[3]);

    int N = 1;
    if (rep_errors[2] < rep_errors[3]) {
        N = 2;
    }

    if (rep_errors[3] < rep_errors[N]) {
        N = 3;
    }

    copy_R_and_t(Rs[N], ts[N], R, t);

    return rep_errors[N];
}


/**
 * @brief 计算矩阵L，论文式13中的L矩阵，不过这里的是按照N=4的时候计算的
 * 
 * @param[in] ut        特征值分解之后得到的12x12的特征矩阵
 * @param[out] l_6x10   计算的L矩阵结果，维度6x10
 */
void PnPsolver::compute_L_6x10(const double* ut, double* l_6x10)
{
    // Step1 获取最后4个零特征值对应的4个12x1的特征向量
    const double* v[4];

    // 对应EPnP里N=4的情况，直接取特征向量的最后4行
    // 以这里的v[0]为例，它是12x1的向量，会拆成4个3x1的向量v[0]^[0], v[0]^[1], v[0]^[3]，对应4个相机坐标系控制点
    v[0] = ut + 12 * 11;    // 最后一行
    v[1] = ut + 12 * 10;    // 倒数第二行
    v[2] = ut + 12 * 9;     // 倒数第三行
    v[3] = ut + 12 * 8;     // 倒数第四行

    // Step2 提前计算中间变量dv
    // dv表示中间变量，是difference-vector的缩写
    // 4表示N=4时对应的4个12x1的向量v，6表示4对点一共有6种两两组合的方式，3表示v^[i]是一个3维的列向量
    double dv[4][6][3];

    // N=4时候的情况，控制第一个下标的就是a，第二个下标的就是b，不过下面的循环中下标都是从0开始
    for (int i = 0; i < 4; i++) {
        int a = 0;
        int b = 1;
        for (int j = 0; j < 6; j++) {
            // dv[i][j] = v[i][a] - v[i][b]
            // a,b的取值有6种组合 0-1 0-2 0-3 1-2 1-3 2-3
            // 假设v数组为[0,1,2,3,4,5,6,7,8,9,10,11]
            // 第一轮：a=0,b=1
            // 0-3, 1-4, 2-5
            // 第二轮：a=0,b=2
            // 0-6, 1-7, 2-8
            // 第三轮：a=0,b=3
            // 0-9, 1-10, 2-11
            // 第四轮：a=1,b=2
            // 3-6, 4-7, 5-8
            // 第5轮：a=1, b=3
            // 3-9, 4-10, 5-11
            // 第6轮：a=2, b=3
            // 6-9, 7-10, 8-11
            dv[i][j][0] = v[i][3*a]     - v[i][3*b];
            dv[i][j][1] = v[i][3*a + 1] - v[i][3*b + 1]
            dv[i][j][2] = v[i][3*a + 2] - v[i][3*b + 2];

            b++;
            if (b > 3) {
                a++;
                b = a + 1;
            }
        }
    }

    // Step3 用前面计算的dv生成L矩阵
    // 这里的6代表前面每个12x1维向量v的4个3x1子向量v[i]对应的6种组合
    for (int i = 0; i < 6; i++) {
        double* row = l_6x10 + 10 * i;
        // 计算每一行中的每一个元素，总共是10个元素
        row[0] =        dot(dv[0][i], dv[0][i]);    // *b11 dv1^2
        row[1] = 2.0f * dot(dv[0][i], dv[1][i]);    // *b12 2dv1·dv2
        row[2] =        dot(dv[1][i], dv[1][i]);    // *b22
        row[3] = 2.0f * dot(dv[0][i], dv[2][i]);    // *b13
        row[4] = 2.0f * dot(dv[1][i], dv[2][i]);    // *b23
        row[5] =        dot(dv[2][i], dv[2][i]);    // *b33
        row[6] = 2.0f * dot(dv[0][i], dv[3][i]);    // *b14
        row[7] = 2.0f * dot(dv[1][i], dv[3][i]);    // *b24
        row[8] = 2.0f * dot(dv[2][i], dv[3][i]);    // *b34
        row[9] =        dot(dv[3][i], dv[3][i]);    // *b44
    }

}


void PnPsolver::compute_rho(double* rho)
{
    rho[0] = dist2(cws[0], cws[1]);
    rho[1] = dist2(cws[0], cws[2]);
    rho[2] = dist2(cws[0], cws[3]);
    rho[3] = dist2(cws[1], cws[2]);
    rho[4] = dist2(cws[1], cws[3]);
    rho[5] = dist2(cws[2], cws[3]);
}


double PnPsolver::dot(const double* v1, const double* v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}


double PnPsolver::dist2(const double* p1, const double* p2)
{
    return (p1[0] - p2[0]) * (p1[0] - p2[0]) +
           (p1[1] - p2[1]) * (p1[1] - p2[1]) +
           (p1[2] - p2[2]) * (p1[2] - p2[2]);
}


void PnPsolver::fill_M(cv::Mat &M, const int row, const double* as, const double u, const double v)
{
    double* M1 = (double*)M.data + row*12;  // 第一行
    double* M2 = M1 + 12;                   // 第二行

    for (int i = 0; i < 4; i++) {   // 4组，每组3个数据
        M1[3*i]     = as[i] * fu;
        M1[3*i + 1] = 0.0f;
        M1[3*i + 2] = as[i] * (uc - u);

        M2[3*i]     = 0.0f;
        M2[3*i + 1] = as[i] * fv;
        M2[3*i + 2] = as[i] * (vc - v);
    }
}


void PnPsolver::add_correspondence(const double X, const double Y, const double Z, const double u, const double v)
{
    pws[3 * number_of_correspondences    ] = X; // 0
    pws[3 * number_of_correspondences + 1] = Y; // 1
    pws[3 * number_of_correspondences + 2] = Z; // 2, pws是一个包含12个元素的数组

    us[2 * number_of_correspondences    ] = u;  // 图像坐标系下的2D点坐标 
    us[2 * number_of_correspondences + 1] = v;

    number_of_correspondences++;
}


void PnPsolver::set_maximum_number_of_correspondences(const int n)  // n == 4
{
    if (maximum_number_of_correspondences < n) {    // 默认为 0 < 4
        if (pws != 0) {
            delete [] pws;
        }

        if (us != 0) {
            delete [] us;
        }
        
        if (alphas != 0) {
            delete [] alphas;
        }

        if (pcs != 0) {
            delete [] pcs;
        }

        maximum_number_of_correspondences = n;
        pws = new double[3 * maximum_number_of_correspondences];    // 3D点在世界坐标系下坐标
        us = new double[2 * maximum_number_of_correspondences];     // 2D点在图像坐标系下的2D点坐标
        alphas = new double[4 * maximum_number_of_correspondences]; // 真实3D点用4个虚拟控制点表达时的系数
        pcs = new double[3 * maximum_number_of_correspondences];    // 3D点在相机坐标系下的坐标
    }
}


void PnPsolver::reset_correspondences(void)
{
    number_of_correspondences = 0;
}


/**
 * @brief 从给定的匹配点中计算出四个控制点
 * 
 */
void PnPsolver::choose_control_points()
{
    // Step1 第一个控制点：参与PnP计算的参考3D点的质心（均值）
    // cws[4][3] 存储控制点在世界坐标系下的坐标，第一维表示是哪个控制点，第二维表示是哪个坐标(x,y,z)
    // 计算前先把第1个控制点坐标清零
    cws[0][0] = cws[0][1] = cws[0][2] = 0;

    // 遍历每个匹配点中世界坐标系3D点，然后对每个坐标轴加和
    // number_of_correspondences默认是4
    for (int i = 0; i < number_of_correspondences; i++) {
        for (int j = 0; j < 3; j++) {
            cws[0][j] += pws[3*i + j];  // pws表示3D点在世界坐标系下的坐标
        }
    }

    // 再对每个轴上取平均值，获得第一个控制点，也就是所有3D点的平均值
    for (int j = 0; j < 3; j++) {
        cws[0][j] /= number_of_correspondences;
    }

    // Step2 计算其它三个控制点，C1,C2,C3通过主成分分解得到
    // ref: https://www.zhihu.com/question/38417101
    // ref: https://yjk94.wordpress.com/2016/11/11/pca-to-layman/
    // 将所有的3D参考点写成矩阵，(number_of_correspondences * 3)的矩阵
    cv::Mat PW0(number_of_correspondences, 3, CV_64F); // 4x3的mat

    double pw0tpw0[3 * 3], dc[3], uct[3 * 3];   // 下面变量的数据区
    cv::Mat PW0tPW0(3, 3, CV_64F, pw0tpw0);     // PW0^T * PW0，为了计算特征值分解
    cv::Mat DC(3, 1, CV_64F, dc);               // 特征值
    cv::Mat UCt(3, 3, CV_64F, uct);             // 特征向量

    // Step2.1 将存在pws中的参考3D点减去第一个控制点（均值中心）的坐标（相当于把第一个控制点作为原点），并存入PW0
    float* ptr = (float*)PW0.data;
    for (int i = 0; i < number_of_correspondences; i++) {
        for (int j = 0; j < 3; j++) {
            ptr[3*i + j] = pws[3*i + j] - cws[0][j];
        }
    }

    // Step2.2 利用特征值分解得到三个主方向
    // PW0^T * PW0
    // cvMulTransposed(A_src, Res_dst, order, delta=null, scale=1)
    // Calculates Res = (A - delta)*(A - delta)^T (order=0) or (A - delta)^T*(A - delta) (order=1)
    // cvMulTransposed(PW0, &PW0tPW0, 1); // 说白了就是计算 PW0^T * PW0
    // 在opencv4.2.5中，用cv::gemm()函数替换,
    // void cv::gemm(
    //     InputArray src1,    // 第一输入矩阵
    //     InputArray src2,    // 第二输入矩阵
    //     double alpha,       // src1 的缩放因子
    //     InputArray src3,    // 第三输入矩阵（可选）
    //     double beta,        // src3 的缩放因子
    //     OutputArray dst,     // 输出矩阵
    //     int flags = 0       // 转置标志位
    // );
    // 转置控制‌：通过 flags 参数灵活控制矩阵转置：
    //  - CV_GEMM_A_T：转置 src1（src1^T）
    //  - CV_GEMM_B_T：转置 src2（src2^T）
    //  - CV_GEMM_C_T：转置 src3（src3^T）
    // dst = alpha * (src1 * src2) + beta * src3
    cv::gemm(PW0, PW0, 1.0, cv::Mat(), 0.0, PW0tPW0, cv::GEMM_1_T); // PW0^T * PW0

    // 特征值分解,cv::SVD::MODIFY_A表示修改矩阵PW0tPW0,直接在输入矩阵 A 上进行奇异值分解，避免额外内存开销。
    // cv::SVD svd(PW0tPW0, cv::SVD::MODIFY_A);
    // // 获取SVD分解结果
    // DC = svd.w;     // 特征值
    // UCt = svd.u;    // 特征向量
    // 使用下面的函数来替代上面svd分解
    cv::SVD::compute(PW0tPW0, DC, UCt, 0, cv::SVD::MODIFY_A);


    // Step2.3 得到C1,C2,C3三个3D控制点，最后加上之前减掉的第一个控制点这个偏移量
    for (int i = 1; i < number_of_correspondences; i++) {
        // 这里只需要遍历后面3个控制点,dc是
        double k = sqrt(dc[i - 1] / number_of_correspondences);
        for (int j = 0; k < 3; j++) {
            cws[i][j] = cws[0][j] + k * uct[3*(i - 1) + j];
        }
    }
}


/**
 * @brief 求解世界坐标系下四个控制点的系数alphas，在相机坐标系下系数不变
 */
void PnPsolver::compute_barycentric_coordinates()
{
    // pws为世界坐标系下3D参考点的坐标
    // cws1 cws2 cws3 cws4为世界坐标系下四个控制点的坐标
    // alphas 四个控制点的系数，每一个pws，都有一组alphas与之对应
    double cc[3 * 3];
    double cc_inv[3 * 3];
    cv::Mat CC(3, 3, CV_64F, cc);           // 除第1个控制点外，另外3个控制点在控制点坐标系下的坐标
    cv::Mat CC_INV(3, 3, CV_64F, cc_inv);   // 上面这个矩阵的逆矩阵

    // Step1 第一个控制点在质心的位置，后面三个控制点减去第一个控制点的坐标（以第一个控制点为原点）
    // 减去质心后得到x,y,z轴
    // cws的排列 |cws1_x cws1_y cws1_z| --> |cws1|
    //           |cws2_x cws2_y cws2_z| --> |cws2|
    //           |cws3_x cws3_y cws3_z| --> |cws3|
    //           |cws4_x cws4_y cws4_z| --> |cws4|
    // 
    // cc的排列 |cc2_x cc3_x cc4_x| --> |cc2 cc3 cc4|
    //          |cc2_y cc3_y cc4_y|
    //          |cc2_z cc3_z cc4_z|
    
    // 将后面3个控制点cws去质心后转化为cc
    for (int i = 0; i < 3; i++) {
        for (int j = 1; j < 4; j++) {
            cc[3 * i + j - 1] = cws[j][i] - cws[0][i];
        }
    }

    // 矩阵求逆，使用SVD分解来求逆
    bool success = cv::invert(CC, CC_INV, cv::DECOMP_SVD);
    if (!success) {
        std::cout << "CC invert failed.";
        return;
    }

    double* ci = cc_inv;
    for (int i = 0; i < number_of_correspondences; i++) {
        double* pi = pws + 3*i;     // pi指向第i个3D点的首地址
        double* a = alphas + 4*i;   // a指向第i个控制点系数alphas的首地址

        // pi[] - cws[0][] 表示去质心
        // a0,a1,a2,a3 对应的是四个控制点的齐次重心坐标
        for (int j = 0; j < 3; j++) {
            a[1 + j] = ci[3*j] * (pi[0] - cws[0][0]) +
                       ci[3*j + 1] * (pi[1] - cws[0][1]) +
                       ci[3*j + 2] * (pi[2] - cws[0][2]);
        }
        // 最后计算用于进行归一化的a0
        a[0] = 1.0f - a[1] - a[2] - a[3];
    }
}


/**
 * @brief 计算N=4时的粗糙近似解，暴力将其它量置为0
 * @param[in] L_6x10    矩阵L
 * @param[in] Rho       非齐次项 ρ，列向量
 * @param[out] betas    计算得到的beta
 * 
 * betas10        = [B11 B12 B22 B13 B23 B33 B14 B24 B34 B44]
 * betas_approx_1 = [B11 B12     B13         B14]
 */
void PnPsolver::find_betas_approx_1(const cv::Mat& L_6x10, const cv::Mat& Rho, double* betas)
{
    // 计算N=4时的粗糙近似解，暴力将其它量置为0
    // betas10        = [B11 B12 B22 B13 B23 B33 B14 B24 B34 B44] -- L_6x10中每一行的内容
    // betas_approx_1 = [B11 B12     B13         B14            ] -- L_6x4中一行提取出来的内容
    double l_6x4[6*4];
    double b4[4];
    cv::Mat L_6x4(6, 4, CV_64F, l_6x4);
    cv::Mat B4(4, 1, CV_64F, b4);

    // 提取L_6x10矩阵中每行的第0，1，3，6个元素，得到L_6x4
    for (int i = 0; i < 6; i++) {
        L_6x4.at<double>(i, 0) = L_6x10.at<double>(i, 0); // 将L_6x10的第i行的第0个元素设置为L_6x4的第i行的第0个元素
        L_6x4.at<double>(i, 1) = L_6x10.at<double>(i, 1);
        L_6x4.at<double>(i, 2) = L_6x10.at<double>(i, 3);
        L_6x4.at<double>(i, 3) = L_6x10.at<double>(i, 6);
    }

    // SVD方式求解方程组 L_6x4 * B4 = Rho
    cv::solve(L_6x4, Rho, B4, cv::DECOMP_SVD);
    // 得到的解是b00 b01 b02 b03 因此解出来b00即可
    if (b4[0] < 0) {
        betas[0] = std::sqrt(-b4[0]);
        betas[1] = -b4[1] / betas[0];
        betas[2] = -b4[2] / betas[0];
        betas[3] = -b4[3] / betas[0];
    } else {
        betas[0] = std::sqrt(b4[0]);
        betas[1] = b4[1] / betas[0];
        betas[2] = b4[2] / betas[0];
        betas[3] = b4[3] / betas[0];
    }
}


/**
 * @brief 计算N=2时的粗糙近似解，暴力将其它量置为0
 * @param[in]  L_6x10   矩阵L
 * @param[in]  Rho      非齐次项ρ，列向量
 * @param[out] betas    计算得到的beta
 * 
 * betas10        = [B11 B12 B22 B13 B23 B33 B14 B24 B34 B44]
 * betas_approx_2 = [B11 B12 B22                            ]
 */
void PnPsolver::find_betas_approx_2(const cv::Mat& L_6x10, const cv::Mat& Rho, double* betas)
{
    double l_6x3[6*3];
    double b3[3];
    cv::Mat L_6x3(6, 3, CV_64F, l_6x3);
    cv::Mat B3(3, 1, CV_64F, b3);

    // 提取
    for (int i = 0; i < 6; i++) {
        L_6x3.at<double>(i, 0) = L_6x10.at<double>(i, 0);
        L_6x3.at<double>(i, 1) = L_6x10.at<double>(i, 1);
        L_6x3.at<double>(i, 2) = L_6x10.at<double>(i, 2);
    }

    // 求解方程组
    cv::solve(L_6x3, Rho, B3, cv::DECOMP_SVD);

    // 从b11 b12 b22中恢复b1 b2
    if (b3[0] < 0) {
        betas[0] = std::sqrt(-b3[0]);
        betas[1] = (b3[2] < 0) ? std::sqrt(-b3[2]) : 0.0;
    } else {
        betas[0] = std::sqrt(b3[0]);
        betas[1] = (b3[2] > 0) ? std::sqrt(b3[2]) : 0.0;
    }
    
    if (b3[1] < 0) {
        betas[0] = -betas[0];
    }

    betas[2] = 0.0;
    betas[3] = 0.0;
}


/**
 * @brief 计算N=3时的粗糙近似解，暴力将其它量置为0
 * @param[in]  L_6x10   矩阵L
 * @param[in]  Rho      非齐次解ρ，列向量
 * @param[out] betas    计算得到的beta
 * 
 * betas10        = [B11 B12 B22 B13 B23 B33 B14 B24 B34 B44]
 * betas_approx_3 = [B11 B12 B22 B13 B23                    ]
 */
void PnPsolver::find_betas_approx_3(const cv::Mat& L_6x10, const cv::Mat& Rho, double* betas)
{
    double l_6x6[6*5];
    double b5[5];
    cv::Mat L_6x5(6, 5, CV_64F, l_6x5)；
    cv::Mat B5(5, 1, CV_64F, b5);

    // 获取并构造矩阵
    for (int i = 0; i < 6; i++) {
        L_6x5.at<double>(i, 0) = L_6x10.at<double>(i, 0);
        L_6x5.at<double>(i, 1) = L_6x10.at<double>(i, 1);
        L_6x5.at<double>(i, 2) = L_6x10.at<double>(i, 2);
        L_6x5.at<double>(i, 3) = L_6x10.at<double>(i, 3);
        L_6x5.at<double>(i, 4) = L_6x10.at<double>(i, 4);
    }

    // 求解方程组
    cv::solve(L_6x5, Rho, B5, cv::DECOMP_SVD);

    // 从B11 B12 B22 B13 B23中恢复出B1 B2 B3
    if (b5[0] < 0) {
        betas[0] = std::sqrt(-b5[0]);
        betas[1] = (b5[2] < 0) ? std::sqrt(-b5[2]) : 0.0;
    } else {
        betas[0] = std::sqrt(b5[0]);
        betas[1] = (b5[2] > 0) ? std::sqrt(b5[2]) : 0.0;
    }

    if (b5[1] < 0) {
        betas[0] = -betas[0];
    }

    betas[2] = b5[3] / betas[0];

    // N=3 时没有B4
    betas[3] = 0.0;
}


/**
 * @brief 对计算出来的beta结果进行高斯牛顿法迭代，求精。过程参考EPnP论文中式(15)
 * @param[in] L_6x10
 * @param[in] Rho
 * @param[in] betas
 */
void PnPsolver::gauss_newton(const cv::Mat& L_6x10, const cv::Mat& Rho, double betas[4])
{
    // 只进行5次迭代
    const int iterations_number = 5;

    /**
     * 这里求解增量方程组 Ax = B，其中x就是增量，根据论文中的式15，可以得到优化的目标函数为：
     * f(β) = Σ||ci - cj||^2
     * Ax = B
     */
    double a[6*4];
    double b[6];
    double x[4];
    cv::Mat A(6, 4, CV_64F, a); // 系数矩阵
    cv::Mat B(6, 1, CV_64F, b); // 非齐次项
    cv::Mat X(4, 1, CV_64F, x); // 增量，待求量

    // 对于每次迭代过程
    for (int k = 0; k < iterations_number; k++) {
        // 计算增量方程的系数矩阵和非齐次项
        compute_A_and_b_gauss_newton(L_6x10, Rho, betas, A, B);
        // 使用QR分解求解增量方程，解得当前次迭代的增量X
        qr_solve(A, B, X);

        // 应用增量，对估计值进行更新；估计值是beta1 ~ beta4组成的向量
        for (int i = 0; i < 4; i++) {
            betas[i] += x[i];
        }
    }
}


/**
 * @brief 根据已经得到的控制点在当前相机坐标系下的坐标来恢复出相机的位姿
 * @param[in]  ut       vi
 * @param[in]  betas    betas
 * @param[out] R        计算得到的相机旋转R
 * @param[out] t        计算得到的相机位置t
 * @return double       使用这个位姿，所得到的重投影误差
 */
double PnPsolver::compute_R_and_t(const double* ut, const double* betas, double R[3][3], double t[3])
{
    // Step1 根据前面的计算结果来“组装”得到控制点在当前相机坐标系下的坐标
    compute_ccs(betas, ut); // 计算ccs
    
    // Step2 将世界坐标系下的3D点的坐标转换到控制点的坐标系下
    compute_pcs();

    // Step3 调整点坐标的符号，来保证相机坐标系下点的深度为正
    solve_for_sign();

    // Step4 ICP计算R和t
    estimate_R_and_t(R, t);

    // Step5 计算使用这个位姿，所得到的每对点平均的重投影误差，作为返回值
    return reprojection_error(R, t);
}


void PnPsolver::compute_ccs(const double* betas, const double* ut)
{
    for (int i = 0; i < 4; i++) {
        ccs[i][0] = ccs[i][1] = ccs[i][2] = 0.0f;
    }

    /**
     * x = \sum_{i=1}^{N} β_i*v_i，这里的x是相机坐标系下控制点坐标
     */

    for (int i = 0; i < 4; i++) {
        const double* v = ut + 12*(11 - i); // 倒数第一行开始
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 3; k++) {
                ccs[j][k] += betas[i] * v[3 * j + k];
            }
        }
    }
}


void PnPsolver::compute_pcs()
{
    for (int i = 0; i < number_of_correspondences; i++) {
        double* a = alphas + 4 * i;
        double* pc = pcs + 3*i;

        for (int j = 0; j < 3; j++) {
            pc[j] = a[0] * ccs[0][j] + a[1] * ccs[1][j] + a[2] * ccs[2][j] + a[3] * ccs[3][j];
        }
    }
}



void PnPsolver::solve_for_sign()
{
    if (pcs[2] < 0.0) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                ccs[i][j] = -ccs[i][j];
            }
        }

        for (int i = 0; i < number_of_correspondences; i++) {
            pcs[3 * i    ] = -pcs[3 * i];
            pcs[3 * i + 1] = -pcs[3 * i + 1];
            pcs[3 * i + 2] = -pcs[3 * i + 2];
        }
    }
}


/**
 * @brief 用3D点在世界坐标系和相机坐标系下对应的坐标，用ICP求解R,t
 * @param[out] R 旋转
 * @param[out] t 平移
 */
void PnPsolver::estimate_R_and_t(double R[3][3], double t[3])
{
    // Step1 计算3D点的质心
    double pc0[3];  // 3D点相机坐标系下坐标的质心
    double pw0[3];  // 3D点世界坐标系下坐标的质心

    // 初始化这两个质心
    pc0[0] = pc0[1] = pc0[2] = 0.0;
    pw0[0] = pw0[1] = pw0[2] = 0.0;

    // 然后累加求质心
    for (int i = 0; i < number_of_correspondences; i++) {
        const double* pc = pcs + 3*i;
        const double* pw = pws + 3*i;

        for (int j = 0; j < 3; j++) {
            pc0[j] += pc[j];
            pw0[j] += pw[j];
        }
    }

    for (int j = 0; j < 3; j++) {
        pc0[j] /= number_of_correspondences;
        pw0[j] /= number_of_correspondences;
    }

    // 准备构造矩阵A,B以及B^T*A的SVD分解的值
    double abt[3*3];
    double abt_d[3];
    double abt_u[3*3];
    double abt_v[3*3];
    cv::Mat ABt(3, 3, CV_64F, abt);     // H = B^t*A
    cv::Mat ABt_D(3, 1, CV_64F, abt_d); // 奇异值分解得到的特征值
    cv::Mat ABt_U(3, 3, CV_64F, abt_u); // 奇异值分解得到的左特征矩阵
    cv::Mat ABt_V(3, 3, CV_64F, abt_v); // 奇异值分解得到的右特征矩阵

    // Step2 构造矩阵H=B^T*A，不过这里是隐含的构造
    cv::setZero(&);
    // 遍历每一个3D点
    for (int i = 0; i < number_of_correspondences; i++) {
        // 定位
        double* pc = pcs + 3*i;
        double* pw = pws + 3*i;

        // 计算H=B^t*A，其中的两个矩阵构造和相乘的操作被融合在一起了
        for (int j = 0; j < 3; j++) {
            abt[3*j    ] += (pc[j] - pc0[j]) * (pw[0] - pw0[0]);
            abt[3*j + 1] += (pc[j] - pc0[j]) * (pw[1] - pw0[1]);
            abt[3*j + 2] += (pc[j] - pc0[j]) * (pw[2] - pw0[2]);
        }
    }

    // Step3 对得到的H矩阵进行奇异值分解
    cv::SVD::compute(ABt, ABt_D, ABt_U, ABt_V, cv::SVD::MODIFY_A);

    // Step4 R=U*V^T,并且进行合法性检查
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            R[i][j] = std::dot(abt_u + 3*i, abt_v + 3*j);
        }
    }

    // 注意在得到了R以后，需要保证 det(R)=1>0
    const double det = R[0][0] * R[1][1] * R[2][2] + R[0][1] * R[1][2] * R[2][0] + R[0][2] * R[1][0] * R[2][1] - 
                       R[0][0] * R[1][2] * R[2][1] - R[0][1] * R[1][0] * R[2][2] - R[0][2] * R[1][1] * R[2][0];
    
    // 如果小于0那么就要加负号
    if (det < 0) {
        R[2][0] = -R[2][0];
        R[2][1] = -R[2][1];
        R[2][2] = -R[2][2];
    }

    // Step5 根据R计算t
    t[0] = pc0[0] - std::dot(R[0], pw0);
    t[1] = pc0[1] - std::dot(R[1], pw0);
    t[2] = pc0[2] - std::dot(R[2], pw0);
}


/**
 * @brief 计算高斯牛顿法优化时，增量方程中的系数矩阵和非齐次项
 * JΔβ=Δy
 * Δy=f(β)=Lβ-ρ
 * 雅可比矩阵J-6x4
 * Δβ-4x1
 * Δy-6x1
 * 
 * @param[in] l_6x10
 * @param[in] rho
 * @param[in] cb
 * @param[out] A
 * @param[out] b
 */
void PnPsolver::compute_A_and_b_gauss_newton(const double* l_6x10, const double* rho, double betas[4], cv::Mat& A, cv::Mat& b)
{
    // 以下推导就是求解一阶雅可比矩阵

    // 一共有六个方程组，对每一行（也就是每一个方程展开遍历）
    // 从优化目标函数的概念出来，其中的每一行的约束均由一对点来提供，因此不同行之间其实并无关系，可以互相独立地计算
    for (int i = 0; i < 6; i++) {
        // 获得矩阵L中的行指针
        const double* rowL = l_6x10 + i*10;
        double* rowA = (double*)A.data + i*4;

        // Step1 计算当前行的雅可比
        /**
         * 雅可比是一个6x4大小的矩阵
         * L1β标识L矩阵的第一行和β的乘积，L是一个6x10的矩阵，β是一个10x1的矩阵,L11表示第一行第一列
         * L1β=[L11 L12 L13 L14 L15 L16 L17 L18 L19 L1x]*[β11 β12 β22 β13 β23 β33 β14 β24 β34 β44]^T
         *    =L11*β11 + L12*β12 + L13*β22 + L14*β13 + L15*β23 + L16*β33 + L17*β14 + L18*β24 + L19*β34 + L1x*β44
         * ə(L1β)/ə(β1) =2L11*β1 +  L12*β2 +  L14*β3 +  L17*β4
         * ə(L1β)/ə(β2) = L12*β1 + 2L13*β2 +  L15*β3 +  L18*β4
         * ə(L1β)/ə(β3) = L14*β1 +  L15*β2 + 2L16*β3 +  L19*β4
         * ə(L1β)/ə(β4) = L17*β1 +  L18*β2 +  L19*β3 + 2L1x*β4
         * 
         */
        rowA[0] = 2 * rowL[0] * betas[0] +     rowL[1] * betas[1] +     rowL[3] * betas[2] +     rowL[6] * betas[3];    // ə(L1β)/ə(β1)
        rowA[1] =     rowL[1] * betas[0] + 2 * rowL[2] * betas[1] +     rowL[4] * betas[2] +     rowL[7] * betas[3];    // ə(L1β)/ə(β2)
        rowA[2] =     rowL[3] * betas[0] +     rowL[4] * betas[1] + 2 * rowL[5] * betas[2] +     rowL[8] * betas[3];    // ə(L1β)/ə(β3)
        rowA[3] =     rowL[6] * betas[0] +     rowL[7] * betas[1] +     rowL[8] * betas[2] + 2 * rowL[9] * betas[3];    // ə(L1β)/ə(β4)

        // Step2 计算当前行的非齐次项
        // 这里就是计算Δy=Lβ-ρ
        // Lβ就是 L11*β11 + L12*β12 + L13*β22 + L14*β13 + L15*β23 + L16*β33 + L17*β14 + L18*β24 + L19*β34 + L1x*β44
        b.at<double>(i, 0) = rho[i] -
                            (
                                rowL[0] * betas[0] * betas[0] +   
                                rowL[1] * betas[0] * betas[1] +
                                rowL[2] * betas[1] * betas[1] +
                                rowL[3] * betas[0] * betas[2] +
                                rowL[4] * betas[1] * betas[2] +
                                rowL[5] * betas[2] * betas[2] +
                                rowL[6] * betas[0] * betas[3] +
                                rowL[7] * betas[1] * betas[3] +
                                rowL[8] * betas[2] * betas[3] +
                                rowL[9] * betas[3] * betas[3]
                            );
    }
}


double PnPsolver::reprojection_error(const double R[3][3], const double t[3])
{
    double sum2 = 0.0;

    for (int i = 0; i < number_of_correspondences; i++) {
        double* pw = pws + 3*i;
        double Xc = std::dot(R[0], pw) + t[0];
        double Yc = std::dot(R[1], pw) + t[1];
        double inv_Zc = 1.0 / (std::dot(R[2], pw) + t[2]);
        double ue = uc + fu * Xc * inv_Zc;
        double ve = vc + fv * Yc * inv_Zc;
        double u = us[2 * i];
        double v = us[2 * i + 1];

        sum2 += std::sqrt((u - ue) * (u - ue) + (v - ve) * (v - ve));
    }

    return sum2 / number_of_correspondences;
}


/**
 * @brief 使用QR分解来求解增量方程
 * @param[in]  A    系数矩阵
 * @param[in]  b    非齐次项
 * @param[out] X    增量
 */
void PnPsolver::qr_solve(cv::Mat& A, cv::Mat& b, cv::Mat& X)
{
    static int max_nr = 0;
    static double *A1, *A2;

    const int nr = A.rows;  // 系数矩阵A的行数
    const int nc = A.cols;  // 系数矩阵A的列数

    // 判断是否需要重新分配A1, A2的内存区域
    if (max_nr != 0 && max_nr < nr) {
        // 如果max_nr != 0，说明之前已经创建了一个last_max_nr < nr的数组，不够我们现在使用了，需要重新分配内存；但是在重新分配之前我们需要先删除之前创建的内容
        delete [] A1;
        delete [] A2;
    }

    if (max_nr < nr) {
        max_nr = nr;
        A1 = new double[nr];
        A2 = new double[nr];
    }

    double *pA = A.ptr<double>();   // 指向系数矩阵A的数据区
    double *ppAkk = pA;             // 一直都会指向对角线上的元素

    // 对系数矩阵的列展开遍历
    for (int k = 0; k < nc; k++) {
        double* ppAik = ppAkk;              // 只是辅助下面的for循环中，遍历对角线元素下的当前列
        double* eta = std::fabs(*ppAik);    // 存储当前列对角线元素下面的所有元素绝对值的最大值

        // 遍历当前对角线约束下，当前列的所有元素，并且找到它们中的最大的绝对值
        for (int i = k + 1; i < nr; i++) {  // 访问行，对角线下方的行
            ppAik += nc;    // 指向下一行
            double elt = std::abs(*ppAik);
            if (eta < elt) {
                eta = elt;
            }
        }   // 找到每列最大值

        if (eta == 0) { // 表示对角线及下方的所有列都为0，那行列式必然为0，是一个奇异矩阵，无法求逆
            A1[k] = A2[k] = 0.0;
            // 奇异矩阵，无法求逆
            std::cerr << "God damn it, A is singular, this shouldn't happen." << std::endl;
            return;
        } else {
            // 开始进行QR分解
            ppAik = ppAkk;  // 重新让ppAik指向对角线元素
            double sum = 0.0;
            double inv_eta = 1. / eta;

            // 对当前列下面的每一行的元素展开遍历（包含对于矩阵主对角线上的元素）
            for (int i = k; i < nr; i++) {
                *ppAik *= inv_eta;      // 这个操作会改变系数矩阵的值，当前指向的元素都会被“归一化”
                sum += *ppAik * *ppAik; // 平方和
                ppAik += nc;            // 指针移动到下一行的这个元素
            }

            // 计算sigma，同时根据对角线元素的符号保持其为正数
            double sigma = std::sqrt(sum);
            if (*ppAkk < 0) {
                sigma = -sigma;
            }

            *ppAkk += sigma;
            A1[k] = sigma * *ppAkk; // A1存储ρ=σ(σ+x1)
            A2[k] = -eta * sigma;   // A2存储σ=η*σ

            // 对于后面的每一列展开遍历
            for (int j = k + 1; j < nc; j++) {  // 遍历第k列之后的每一列
                ppAik = ppAkk;  // 将ppAik重新指向对角线元素 A[k][k]
                sum = 0;
                // 遍历列的每一行
                for (int i = k; i < nr; i++) { // 计算当前列与对角线列的内积
                    sum += *ppAik * ppAik[j -  k];  // 计算当前列与对角线列的内积,计算当前列（索引j）与对角线列（索引k）的点积, *ppAik是A[i][k]，ppAik[j-k]是A[i][j]
                    ppAik += nc;                    // 跳到下一行
                }
                double tau = sum / A1[k];   // 计算豪斯霍尔德反射系数τ = σ/ρ
                // 然后再一遍循环是为了修改
                ppAik = ppAkk;
                for (int i = k; i < nr; i++) {
                    ppAik[j - k] -= tau * *ppAik;   // 应用反射变换：A[j][i] -= τ * A[k]
                    ppAik += nc;
                }
            }
        }
        // 移动向下一个对角线元素
        ppAkk += nc + 1;

        // b <- Qt b
        double* ppAjj = pA;
        double* pb = b.ptr<double>();
        // 对每一列展开计算
        for (int j = 0; j < nc; j++) {
            // 这个部分在计算Q^T*b
            double* ppAij = ppAjj;
            double tau = 0;
            for (int i = j; i < nr; i++) {
                tau += *ppAij * pb[i];
                ppAij += nc;
            }
            tau /= A1[j];
            ppAij = ppAjj;
        }

    }
}
}