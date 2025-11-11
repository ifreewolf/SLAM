#!/bin/bash

export LD_LIBRARY_PATH="./3rdparty/pangolin/lib":${LD_LIBRARY_PATH}

./bin/rgbd_tum Vocabulary/ORBvoc.txt Examples/RGBD/TUM1.yaml data/rgbd_dataset_freiburg1_xyz/ Examples/RGBD/associations/fr1_xyz.txt