# 1. `ComputeSim3`函数

- 输入：`P1`和`P2`，表示匹配的3D点，其中`P1`是当前帧的地图点，`P2`是闭环关键帧的地图点

```bash
1. 定义3D点质心及去质心后的点
    cv::Mat Pr1(P1.size(), P1.type())   # 减去质心后的3D点
    cv::Mat Pr2(P2.size(), P2.type())   # 减去质心后的3D点
    cv::Mat O1(3, 1, Pr1.type())        # 质心
    cv::Mat O2(3, 1, Pr2.type())        # 质心
2. 计算M矩阵
    cv::Mat M = Pr2 * Pr1.t()
3. 计算N矩阵
    cv::Mat N(4, 4, P1.type())、
    double N11, N12, N13, N14, N22, N23, N24, N33, N34, N44
    N11 = M.at<float>(0, 0) + M.at<float>(1, 1) + M.at<float>(2, 2);    // Sxx + Syy + Szz
    N12 = M.at<float>(1, 2) - M.at<float>(2, 1);                        // Syz - Szy
    N13 = M.at<float>(2, 0) - M.at<float>(0, 2);                        // Szx - Sxz
    N14 = M.at<float>(0, 1) - M.at<float>(1, 0);                        // Sxy - Syx
    N22 = M.at<float>(0, 0) - M.at<float>(1, 1) - M.at<float>(2, 2);    // Sxx - Syy - Szz
    N23 = M.at<float>(0, 1) - M.at<float>(1, 0);                        // Sxy + Syx
    N24 = M.at<float>(2, 0) + M.at<float>(0, 2);                        // Szx + Sxz
    N33 = -M.at<float>(0, 0) + M.at<float>(1, 1) - M.at<float>(2, 2);   // -Sxx + Syy - Szz
    N34 = M.at<float>(1, 2) + M.at<float>(2, 1);                        // Syz + Szy
    N44 = -M.at<float>(0, 0) - M.at<float>(1, 1) + M.at<float>(2, 2);   // -Sxx - Syy + Szz
    N = (cv::Mat_<float>(4, 4) << N11, N12, N13, N14,
                                  N12, N22, N23, N24,
                                  N13, N23, N33, N34,
                                  N14, N24, N34, N44);
4. 特征值分解求最大特征值对应的特征向量，也就是旋转四元数
    cv::Mat eval, evec
    cv::eigen(N, eval, evec)    # evec[0] is the quaternion of the desired rotation
    # N矩阵最大特征值（第一个特征值）对应特征向量就是要求的四元数（q0, q1, q2, q3），其中q0是实部
    cv::Mat vec(1, 3, evec.type())
    (evec.row(0).colRange(1, 4)).copyTo(vec) # 将(q1 q2 q3)放入vec（四元数的虚部）
    计算角度，四元数虚部模长 norm(vec) = sin(theta / 2)，四元数实部 evec.at<float>(0, 0) = q0 = cos(theta / 2)
    double ang = std::atan2(std::norm(vec), evec.at<float>(0, 0)) # tan(theta/2) = norm(vec)/q0
    vec/norm(vec)归一化得到归一化后的旋转向量，然后乘上角度得到包含了旋转轴和旋转角信息的旋转向量vec
    vec = 2 * ang * vec / std::norm(vec)

    旋转向量(轴角)转换为旋转矩阵
    cv::Rodrigues(vec, mR12i)
5. 将三维点旋转到同一个坐标系下，将P2地图点转换到P1坐标系下
    cv::Mat P3 = mR12i * Pr2
6. 计算尺度因子 Scale，使用非对称方式
    double nom = Pr1.dot(P3)
    准备计算分量
    cv::Mat aux_P3(P3.size(), P3.type())
    先得平方，矩阵逐个元素求和
    cv::pow(P3, 2, aux_P3)
    然后再累加，对矩阵的逐个元素进行求和
    for (int i = 0; i < aux_P3.rows; i++) {
        for (int j = 0; j < aux_P3.cols; j++) {
            den += aux_P3.at<float>(i, j);
        }
    }
    # 计算尺度,论文里的方案
    # nom‌：计算的是两个坐标系中匹配点集去中心化后的坐标的模长乘积之和
    # den‌：计算的是源坐标系中点集去中心化后的坐标的模长平方之和
    ms12i = nom / den;
7. 计算平移Translation
    mt12i.create(1, 3, P1.type())
    mt12i = O1 - ms12i * mR12i * O2 # 论文中平移公式,就是转换到P1坐标系,然后算上尺度计算平移
8. 计算双向变换矩阵
    用尺度,旋转,平移构建变换矩阵T12
    mT12i = cv::Mat::eye(4, 4, P1.type())
    cv::Mat sR = ms12i * mR12i
    sR.copyTo(mT12i.rowRange(0, 3).colRange(0, 3))
    mt12i.copyTo(mT12i.rowRange(0, 3).col(3))

    计算T21,计算公式:
    
    mT21i = cv::Mat::eye(4, 4, P1.type())
    cv::Mat sRinv = (1.0 / ms12i) * mR12i.t()
    sRinv.copyTo(mT21i.rowRange(0, 3).colRange(0, 3))
    cv::Mat tinv = -sRinv * mt12i;
    tinv.copyTo(mT21i.rowRange(0, 3).col(3))
```