#include "common.h"
#include "System.h"

void LoadImages(const std::string &strAssociationFilename, std::vector<std::string> &vstrImageFilenamesRGB, std::vector<std::string> &vstrImageFilenamesD, std::vector<double> &vTimestamps);

int main(int argc, char **argv)
{
    if (argc != 5) {
        std::cerr << std::endl << "Usage: ./rgbd_tum path_to_vocabulary path_to_settings path_to_sequence path_to_association" << std::endl;
        return 1;
    }

    // Step1 读取图片及左右目关联信息
    std::vector<std::string> vstrImageFilenamesRGB;
    std::vector<std::string> vstrImageFilenamesD;
    std::vector<double> vTimestamps;
    std::string strAssociationFilename = std::string(argv[4]);
    LoadImages(strAssociationFilename, vstrImageFilenamesRGB, vstrImageFilenamesD, vTimestamps);

    // Step2 检查图片文件与输入文件的一致性
    int nImages = vstrImageFilenamesRGB.size();
    if (vstrImageFilenamesRGB.empty()) {
        std::cerr << std::endl << "No images found in provided path." << std::endl;
        return 1;
    } else if (vstrImageFilenamesD.size() != vstrImageFilenamesRGB.size()) {
        std::cerr << std::endl << "Different number of images for rgb and depth." << std::endl;
        return 1;
    }

    // Step3 创建SLAM对象，它是一个 ORB_SLAM2::System 类型变量
    // strVocFile, strSettingsFile, sensor, bUseViewer
    ORB_SLAM2::System SLAM(argv[1], argv[2], ORB_SLAM2::System::RGBD, false);

    // Vector for tracking time statistics
    std::vector<float> vTimesTrack;
    vTimesTrack.resize(nImages);

    std::cout << std::endl << "-------" << std::endl;
    std::cout << "Start processing sequence ..." << std::endl;
    std::cout << "Images in the sequence: " << nImages << std::endl << std::endl;

    // Main loop
    cv::Mat imRGB, imD;
    // Step4 遍历图片，进行SLAM
    for (int ni = 0; ni < nImages; ni++) {
        // Step4.1 读取图片
        imRGB = cv::imread(std::string(argv[3]) + "/" + vstrImageFilenamesRGB[ni], cv::IMREAD_UNCHANGED);
        imD   = cv::imread(std::string(argv[3]) + "/" + vstrImageFilenamesD[ni], cv::IMREAD_UNCHANGED);
        double tframe = vTimestamps[ni];

        if (imRGB.empty()) {
            std::cerr << std::endl << "Failed to load image at: "
                      << std::string(argv[3]) << "/" << vstrImageFilenamesRGB[ni] << std::endl;
            return 1;
        }

        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

        // Step4.2 进行SLAM
        SLAM.TrackRGBD(imRGB, imD, tframe);

        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

        // Track的时间间隔，单位是秒
        double ttrack = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();

        vTimesTrack[ni] = ttrack;

        // Step4.3 加载下一张图片
        double T = 0;   // 获取两帧之间的间隔，单位为秒
        if (ni < nImages - 1) { // (0-n-1)帧图片
            T = vTimestamps[ni + 1] - tframe; // 两帧之间的间隔
        } else if (ni > 0) {    // 最后一帧
            T = tframe - vTimestamps[ni - 1];
        }

        if (ttrack < T) {   // ttrack是Track耗时，单位为秒；如果Track耗时小于俩帧之间的间隔，则睡眠等待
            std::this_thread::sleep_for(std::chrono::microseconds(static_cast<unsigned int>((T - ttrack)*1e6)));
        }
    }

    // Step5 停止SLAM
    SLAM.Shutdown();

    // Tracking time statistics
    // 分析跟踪时间
    sort(vTimesTrack.begin(), vTimesTrack.end()); // 从小到大排序
    float totaltime = 0;
    for (int ni = 0; ni < nImages; ni++) {
        totaltime += vTimesTrack[ni];
    }

    std::cout << "----------" << std::endl << std::endl;
    std::cout << "median tracking time: " << vTimesTrack[nImages/2] << std::endl;
    std::cout << "mean tracking time: " << totaltime / nImages << std::endl;

    // Save camera trajectory
    SLAM.SaveTrajectoryTUM("CameraTrajectory.txt");
    SLAM.SaveKeyFrameTrajectoryTUM("KeyFrameTrajectory.txt");

    return 0;
}

void LoadImages(const std::string &strAssociationFilename, std::vector<std::string> &vstrImageFilenamesRGB, std::vector<std::string> &vstrImageFilenamesD, std::vector<double> &vTimestamps)
{
    std::ifstream fAssociation;
    fAssociation.open(strAssociationFilename.c_str());
    while (!fAssociation.eof()) {
        std::string s;
        getline(fAssociation, s);
        if (!s.empty()) {
            std::stringstream ss;
            ss << s;
            double t;
            std::string sRGB, sD;
            ss >> t;
            vTimestamps.emplace_back(t);
            ss >> sRGB;
            vstrImageFilenamesRGB.emplace_back(sRGB);
            ss >> t;
            ss >> sD;
            vstrImageFilenamesD.emplace_back(sD);
        }
    }
}
