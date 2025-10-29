# 一：`SearchByProjection(Frame &CurrentFrame, const Frame &LastFrame, const float th, const bool bMono)`

```bash
目的是将上一帧跟踪的地图点投影到当前帧，并且搜索匹配点，用于跟踪前一帧

如何将上一帧跟踪的地图点投影到当前帧？

循环遍历上一帧的特征点
for (int i = 0; i < LastFrame.N; i++)
    获取特征点对应的地图点
    MapPoint* pMP = LastFrame.mvpMapPoints[i]
    该地图点存在，且该特征点不是外点
    if (pMP && !LastFrame.mvbOutlier[i])
        获取该地图点在世界坐标系下的坐标
        cv::Mat x3Dw = pMP->GetWorldPos()
        上一帧地图点在当前帧相机坐标系下的坐标
        cv::Mat x3Dc = Rcw*x3Dw + tcw

        获取上一帧地图点在当前帧的像素坐标
        const float xc = x3Dc.at<float>(0)
        const float yc = x3Dc.at<float>(1)
        const float invzc = 1.0 / x3Dc.at<float>(2)
        float u = CurrentFrame.fx * xc * invzc + CurrentFrame.cx
        float v = CurrentFrame.fy * yc * invzc + CurrentFrame.cy

        获得上一帧地图点在当前帧的像素坐标后，在当前帧(u,v)处搜索候选特征点
        vIndices2 = CurrentFrame.GetFeaturesInArea(u, v, radius, nLastOctave)

        获得上一帧特征点对应的描述子
        const cv::Mat dMP = pMP->GetDescriptor()

        循环遍历当前帧候选特征点
        for (size_t i2 : vIndices2)
            获得候选特征点的描述子
            const cv::Mat &d = CurrentFrame.mDescriptors.row(i2)

            计算前一帧和当前帧候选特征点描述子的距离
            const int dist = DescriptorDistance(dMP, d)

            记录候选特征点中最小距离的特征点
            if (dist < bestDist)
                bestDist = dist
                bestIdx2 = i2
        
        判断最佳匹配距离小于设定阈值
        if (bestDist <= TH_HIGH)
            选择最佳匹配特征点就是上一帧对应的地图点
            CurrentFrame.mvpMapPoints[bestIdx2] = pMP

            匹配数量加1
            nmatches++

            计算匹配点旋转角度差所在的直方图
            float rot = LastFrame.mvKeysUn[i].angle - CurrentFrame.mvKeysUn[bestIdx2].angle
            rotHist[bin].push_back(bestIdx2)
进行旋转一致性检查，只保留前三个匹配最多的角度
ComputeThreeMaxima(rotHist, HISTO_LENGTH, ind1, ind2, ind3)
if (i != ind1 && i != ind2 && i != ind3)
    CurrentFrame.mvpMapPoints[rotHist[i][j]] = static_cast<MapPoint*>(NULL)
    nmatches--
return nmatches;
```