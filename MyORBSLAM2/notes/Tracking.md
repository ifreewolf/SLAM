# 一、Tracking流程

```bash
可执行程序-->System构造函数(初始化三个线程)-->处理输入的帧(TrackMonocular)-->调用Tracking线程中的GrabImageXXX函数
```

追踪线程的主函数(三选一)：

1. GrabImageRGBD
2. GrabImageMonocular
3. GrabImageStereo

## 1.1 主要流程

可执行程序-->System构造函数(初始化三个线程)-->system中的RGBD初始化函数(TrackRGBD)-->Tracking线程(GrabImageRGBD)
(1) 灰度化处理
(2) 构建当前帧(在这里会提取特征点，并将特征点分配到网格里)
(3) 进入Track函数-->Track函数(Track())

## 1.2 `Track()`函数

### 1.2.1 初始化
#### 1.2.1.1 单目初始化(`MonocularInitialization`)

(1) 特征点匹配，如果匹配数目不够的话，等待后两帧。(`SearchForInitialization`)
(2) 并行计算H、F矩阵。H矩阵采用(RANSAC+DLC)，F矩阵(RANSAC+八点法)
(3) 根据评分(重投影误差)选择F/H
(4) 分解F/H，选择最合适的R,t（通过统计合理的路标点的数量，如：视差、重投影误差：Z>0）
(5) 三角化估计路标点深度信息。(2,3,4,5都在(`Initialize`)函数中实现)
(6) 全局BA优化，进行尺度归一化。(`CreateInitialMapMonocular`)

#### 1.2.1.2 双目和RGBD初始化(`StereoInitialization`)

(1) 创建关键帧。
(2) 获取特征点的3D点
(3) 根据获取的特征点的3D点创建地图点
(4) 地图点、关键帧属性设置

### 1.2.2 使用追踪方式获得优化后的相机位姿

#### 1.2.2.1 恒速运动模型：`TrackWithMotionModel`

- 通过前两帧的位姿变换估计当前位姿，使用领域半径搜索的方法获得匹配点对(`SearchByProjection`)，随后优化位姿(`PoseOptimization`)



#### 1.2.2.2 参考关键帧模型：`TrackReferenceKeyFrame`

- 以上一帧的位姿为初始位姿，找到当前帧的参考关键帧，使用词袋加速算法获得特征点对(`SearchByBoW`)，随后进行位姿优化(`PoseOptimization`)

#### 1.2.2.3 重定位模型：`Relocalization`

- 使用词袋加速算法获得候选关键帧(`DetectRelocalizationCandidates`,`SearchByBoW`)，使用EPnP估计相机位姿(`SetRansacParameters`)，如果内点数不够的话，将候选关键帧的路标点投影到当前帧中(`SearchByProjection`)，获得更多的匹配点，再次进行优化

### 1.2.3 追踪局部地图（`TrackLocalMap`）

#### 1.2.3.1 获得局部关键帧(有共视关系的帧及其邻居)和局部路标点

#### 1.2.3.2 优化位姿。如果跟踪失败的话，Tracking线程返回false

### 1.2.4 判断关键帧(`NeedNewKeyFrame`)

- 必要条件：1）内点数目不能小于15；2）内点数目小于参考关键帧的一定的比例；以下条件必须满足一个：1）很长时间没有插入关键帧；2）满足插入关键帧的最小间隔并且`LocalMapper`处于空闲状态。
