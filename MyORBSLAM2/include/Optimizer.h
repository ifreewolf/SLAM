#pragma once
#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "common.h"
#include "LoopClosing.h"
#include "Frame.h"

namespace ORB_SLAM2
{
class LoopClosing;

class Optimizer
{
public:
    int static PoseOptimization(Frame* pFrame);
};
}

#endif
