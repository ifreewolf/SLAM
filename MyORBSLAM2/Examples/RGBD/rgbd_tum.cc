#include "common.h"
#include "System.h"

void LoadImages(const std::string &strAssociationFilename, std::vector<std::string> &vstrImageFilenamesRGB, std::vector<std::string> &vstrImageFilenamesD, std::vector<double> &vTimestamps);

int main(int argc, char **argv)
{
    if (argc != 5) {
        std::cerr << std::endl << "Usage: ./rgbd_tum path_to_vocabulary path_to_settings path_to_sequence path_to_association" << std::endl;
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
        
    }
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
