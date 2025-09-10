#include "System.h"

namespace ORB_SLAM2
{

System::System(const std::string &strVocFile, const std::string &strSettingsFile, const eSensor sensor, const bool bUseViewer)
    : mSensor(sensor)
{
    std::cout << "System ctor" << std::endl;
}


}