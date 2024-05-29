#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <Eigen/Core>
#include <pangolin/pangolin.h>
#include <unistd.h>

string left_file = "./left.png";
string right_file = "./right.png";

void showPointCloud(const vector<Vector4d, Eigen::aligned_allocator<Vector4d>> &pointcloud);

int main(int argc, char **argv)
{
    // 内参
    double fx = 718.856, fy = 718.856, cx = 607.1928, cy = 185.2157;
    // 基线
    double b = 0.573;

    cv::Mat left = cv::imread(left_file, 0);
    cv::Mat right = cv::imread(right_file, 0);

    cv::Ptr<cv::StereoSGBM> sgbm = cv::StereoSGBM::create(0, 96, 9, 8 * 9 * 9, 32 * 9 * 9, 1, 63, 10, 100, 32);

    cv::Mat disparity_sgbm, disparity;
    sgbm->compute(left, right, disparity_sgbm);

    // 生成点云
    vector<Vector4d, Eigen::aligned_allocator<Vector4d>> pointcloud;
    for (int v = 0; v < left.rows; v++) {
        for (int u = 0; u < left.cols; u++) {
            if (disparity.at<float>(v, u) <= 0.0 || disparity.at<float>(v, u) >= 96.0) continue;

            Vector4d point(0, 0, 0, left.at<uchar>(v, u) / 255.0);  // 前三维为xyz, 第四维为颜色

            // 根据双目模型计算 point 的位置
            double x = (u - cx) / fx;
            double y = (v - cy) / fy;
            double depth = fx * b / (disparity.at<float>(v, u));
            point[0] = x * depth;
            point[1] = y * depth;
            point[2] = depth;

            pointcloud.push_back(point);
        }
    }

    cv::imshow("disparity", disparity / 96.0);
    cv::waitKey(0);

    // 画出点云
    showPointCloud(pointcloud);
    
    return 0;
}