 ### 1.开发测试 RGBD
 ```bash
 ./bin/rgbd_tum Examples/RGBD/associations/fr1_desk2.txt data/ data/ Examples/RGBD/associations/fr1_desk2.txt
 ```

 ## 双目运行

 ```bash
 ./bin/rgbd_tum Vocabulary/ORBvoc.txt Examples/RGBD/TUM1.yaml data/rgbd_dataset_freiburg1_xyz/ Examples/RGBD/associations/fr1_xyz.txt
 ```

 - TUM1.yaml 对应 freiburg1序列；TUM2.yaml 对应 freiburg2序列；TUM3.yaml 对应 freiburg3序列
 - freiburg1包含办公室/桌面场景数据；freiburg2包含大范围室内环境数据；freiburg3包含长距离家庭/办公室场景数据
 - Trajectory：轨迹