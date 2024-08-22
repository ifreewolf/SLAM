#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/feature2d/feature2d.hpp>
#include <opencv2/imgcodecs/legacy/constants_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <Eigen/core>
#include <g2o/core/base_vertex.h>
#include <g2o/core/base_unary_edge.h>
#include <g2o/core/sparse_optimizer.h>
#include <g2o/core/block_solver.h>
#include <g2o/core/solver.h>
#include <g2o/core/optimization_algorithm_gauss_newton.h>
#include <g2o/solvers/dense/linear_solver_dense.h>
#include <sophus/se3.hpp>
#include <chrono>

using namespace std;
using namespace cv;

void find_feature_matches(
    const Mat &img_1, const Mat &img_2,
    std::vector<KeyPoint> &keypoints_1,
    std::vector<KeyPoint> &keypoints_2,
    std::vector<DMatch> &matches
);

// 像素坐标转相机归一化坐标
Point2d pixel2cam(const Point2d &p, const Mat &K);

// BA by g2o
typedef vector<Eigen::Vector2d, Eigen::aligned_allocator<Eigen::Vector2d>> VecVector2d;
typedef vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d>> VecVector3d;

void bundleAdjustmentG2O(
    const Vector3d &points_3d,
    const Vector2d &points_2d,
    const Mat &K,
    Sophus::SE3d &pose
);

int main(int argc, char **argv)
{
    if (argc != 5) {
        std::cout << "usage: pose_estimation_3d2d img1 img2 depth1 depth2" << std::endl;
        return 1;
    }
    // 读取图像
    Mat img_1 = imread(argv[1], CV_LOAD_IMAGE_COLOR);
    Mat img_2 = imread(argv[2], CV_LOAD_IMAGE_COLOR);
    assert(img_1.data && img_2.data && "Can not load images!");

    vector<KeyPoint> keypoints_1, keypoints_2;
    vector<DMatch> matches;
    find_feature_matches(img_1, img_2, keypoints_1, keypoints_2, matches);
    std::cout << "一共找到了" << matches.size() << "组匹配点" << std::endl;

    // 建立3D点
    Mat d1 = imread(argv[3], CV_LOAD_IMAGE_UNCHANGED);  // 深度图为16位无符号数，单通道图像
    return 0;
}

/*
https://blog.csdn.net/u011341856/article/details/107758182

超定方程A x = 0 \bm A\bm x =0Ax=0的解，就是对A \bm AA进行SVD分解之后的最小特征值对应的特征向量。
*/