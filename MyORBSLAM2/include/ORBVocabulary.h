#pragma once
#ifndef ORBVOCABULARY_H
#define ORBVOCABULARY_H

#include "3rdparty/DBoW2/DBoW2/FORB.h"
#include "3rdparty/DBoW2/DBoW2/TemplatedVocabulary.h"

namespace ORB_SLAM2
{

typedef DBoW2::TemplatedVocabulary<DBoW2::FORB::TDescriptor, DBoW2::FORB> ORBVocabulary;

}

#endif