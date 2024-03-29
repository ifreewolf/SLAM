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

只要求出了两组点之间的旋转，平移量是非常容易得到的，所以重点关注$R$的计算，展开关于$R$的误差项，得

$$
\frac{1}{2}\sum_{i=1}^{n}\lVert q_i - R q_i' \rVert^2 = \frac{1}{2}\sum_{i=1}^n(q_i^Tq_i + q_i'^TR^TRq_i'-2q_i^TRq_i'). \tag{7.55}
$$

注意到第一项和$R$无关，第二项由于$R^TR=I$，亦与$R$无关，因此，实际上优化目标函数变为

$$
\sum_{i=1}^n -q_i^TRq_i'=\sum_{i=1}^n -tr(Rq_i'q_i^T) = -tr\left( R\sum_{i=1}^nq_i'q_i^T \right). \tag{7.56}
$$

接下来，介绍怎样通过SVD解出上述问题中最优的$R$，为了解$R$，先定义矩阵：

$$
W = \sum_{i=1}^n q_iq_i^{'T}. \tag{7.57}
$$

$W$是一个$3\times3$的矩阵，对$W$进行SVD分解，得

$$
W = U\Sigma V^T. \tag{7.58}
$$

其中，$\Sigma$为奇异值组成的对角矩阵，对角线元素从大到小排列，而$U$和$V$为对角矩阵，当$W$满秩时，$R$为

$$
R = UV^T. \tag{7.59}
$$

解得$R$后，按式(7.54)求解$t$即可，如果此时$R$的行列式为负，则取$-R$作为最优值。

---

## 7.9.2 非线性优化方法

求解ICP的另一种方式是使用非线性优化，以迭代的方式去找最优值，该方法和前面讲述的PnP非常相似，以李代数表达位姿时，目标函数可以写成

$$
\min_{\xi} = \frac{1}{2}\sum_{i=1}^n \lVert (p_i - \exp(\xi^{\wedge})p_i') \rVert_2^2. \tag{7.60}
$$

单个误差项关于位姿的导数在前面已推导，使用李代数扰动模型即可：

$$
\frac{\partial e}{\partial \delta \xi} = -(\exp(\xi^{\wedge})p_i')^{\odot}. \tag{7.61}
$$

于是，在非线性优化中只需不断迭代，就能找到极小值。而且，可以证明，ICP问题存在唯一解或无穷多解的情况。在唯一解的情况下，只要能找到极小值解，<B>这个极小值就是全局最优值</B>--因此不会遇到局部极小而非全局最小的情况。这也意味着ICP求解可以任意选定初始值。这是已匹配点时求解ICP的一大好处。

需要说明的是，这里讲的ICP是指已由图像特征给定了匹配的情况下进行位姿估计的问题，在匹配已知的情况下，这个最小二乘问题实际上具有解析解，所以并没有必要进行迭代优化。

ICP的研究者们往往更加关心匹配未知的情况，那么，为什么要介绍基于优化的ICP呢？这是因为，某些场合下，例如在RGB-D SLAM中，一个像素的深度数据可能有，也可能测量不到，所以可以混合着使用PnP和ICP优化：对于深度已知的特征点建模它们的3D-3D误差；对于深度未知的特征点，则建模3D-2D的重投影误差。于是，可以讲所有的误差放在同一个问题中考虑，使得求解更加方便。
