# Chapter7.9

# 7.9 3D-3D：ICP

假设有一组配对好的3D点(例如我们对两幅RGB-D图像进行匹配)：

$$
P = {p_1, \cdots, p_n}, P'={p_1', \cdots, p_n'},
$$

现在，想要找一个欧氏变换$R,t$，使得

$$
\forall i, p_i = R p_i' + t.
$$

这个问题可以用迭代最近点(Interative Closest Point, ICP)求解。3D-3D位姿估计问题中并没有出现相机模型，也就是说，仅考虑两组3D点之间的变换时，和相机并没有关系。因此，在激光SLAM中也会碰到ICP，不过由于激光数据特征不够丰富，无从知道两个点集之间的匹配关系，只能认为距离最近的两个点为同一个，所以这个方法称为迭代最近点。

而在视觉中，特征点为我们提供了较好的匹配关系，所以整个问题就变得更简单了。在RGB-D SLAM中，可以用这种方式估计相机位姿。下文，我们用ICP指代匹配好的两组点间的运动估计问题。

和PnP类似，ICP的求解也分为两种方式：利用线性代数的求解(主要是SVD)，以及利用非线性优化方式的求解(类似于BA)。下面分别介绍。

## 7.9.1 SVD方法

根据前面描述的ICP问题，先定义第$i$对点的误差项：

$$
e_i = p_i - (Rp_i' + t). \tag{7.49}
$$

然后，构建最小二乘问题，求使误差平方和达到极小的$R,t$:

$$
\min_{R,t}\frac{1}{2}\sum_{i=1}^{n}\lVert (p_i - (Rp_i' + t)) \rVert_2^2. \tag{7.50}
$$

下面推导它的求解方法，首先，定义两组点的质心：

$$
p = \frac{1}{n}\sum_{i=1}^{n}(p_i), \quad p'=\frac{1}{n}\sum_{i=1}^{n}(p_i'). \tag{7.51}
$$

质心是没有下标的，随后，在误差函数中做如下处理：

$$
\begin{aligned}
    \frac{1}{2}\sum_{i=1}^n\lVert p_i - (Rp_i' + t) \rVert^2 &= \frac{1}{2}\sum_{i=1}^{n}\lVert p_i - Rp_i' - t - p + Rp' + p - Rp' \rVert^2 \\
    &= \frac{1}{2}\sum_{i=1}^{n}\lVert (p_i - p - R(p_i' - p')) + (p - Rp' - t) \rVert^2 \\
    &= \frac{1}{2}\sum_{i=1}^{n}(\lVert p_i - p - R(p_i' - p') \rVert^2 + \lVert p - Rp' - t \rVert^2 + \\
    &\qquad 2(p_i - p - R(p_i' - p'))^T(p - Rp' - t)).
\end{aligned}
$$

注意到交叉项部分中$(p_i - p - R(p_i'-p'))$在求和之后为零，因此优化目标函数可以简化为

$$
\min_{R,t} J = \frac{1}{2}\sum_{i=1}^{n}\lVert p_i - p - R(p_i' - p') \rVert^2 + \lVert p - Rp' - t\rVert^2. \tag{7.52}
$$

仔细观察左右两项，发现左边只和旋转矩阵$R$相关，而右边既有$R$也有$t$，但只和质心有关。只要我们获得了$R$，令第二项为零就能得到$t$。于是，ICP可以分为以下三个步骤求解：

<div style="border: 1px solid black;">
1. 计算两组点的质心位置$p, p'$，然后计算每个点的<B>去质心坐标</B>：

$$q_i = p_i - p,  \quad q_i' = p_i' - p'.$$

2. 根据以下优化问题计算旋转矩阵：

$$
R^* = \argmin_{R}\frac{1}{2}\sum_{i=1}^n \lVert q_i - Rq_i' \rVert^2. \tag{7.53}
$$

3. 根据第2步的$R$计算$t$:

$$
t^* = p - Rp'. \tag{7.54}
$$
</div>

