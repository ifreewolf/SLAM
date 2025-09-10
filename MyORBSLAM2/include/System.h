#pragma once
#ifndef SYSTEM_H
#define SYSTEM_H

#include "common.h"

namespace ORB_SLAM2{

class System
{
public:
    // Input sensor
    enum eSensor{
        MONOCULAR=0,
        STEREO=1,
        RGBD=2
    };
public:
    System(const std::string &strVocFile, const std::string &strSettingsFile, const eSensor sensor, const bool bUseViewer = true);

private:
    // Input sensor
    eSensor mSensor;
};

}

#endif