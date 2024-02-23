# 李群与李代数

<B>主要目标</B>

> 1. 理解李群与李代数的概念，掌握SO(3)、SE(3)与对应李代数的表示方式。
> 2. 理解BCH近似的意义。
> 3. 学会在李代数上的扰动模型。
> 4. 使用Sophus对李代数进行运算。

第3讲介绍了旋转的表示，但在SLAM中，除了表示，还需要对它们进行估计和优化。在SLAM中，位姿是未知的，SLAM需要解决形如“<B>什么样的相机位姿最符合当前观测数据</B>”这样的问题。一种典型的方式是把它构建成一个优化问题，求解最优的$R,t$，使得误差最小化。

旋转矩阵自身是带有约束的(正交且行列式为1).它们作为优化变量，会引入额外的约束，使优化变得困难。通过李群--李代数间的转换关系，我们希望把位姿估计变成无约束的优化问题，简化求解方式。

# 4.1 李群与李代数基础

三维旋转矩阵构成了<B>特殊正交群SO(3)</B>，而变换矩阵构成了<B>特殊欧氏群SE(3)</B>。

$$
SO(3)=\{R\in \mathbb{R}^3 | RR^T=I, det(R) = 1 \}, \tag{4.1}
$$

$$
SE(3)=\left\{ T=\begin{bmatrix} R & t \\ 0^T & 1 \end{bmatrix} \in \mathbb{R}^{ 4\times 4} |R \in SO(3), t\in \mathbb{R}^3 \right\}. \tag{4.2}
$$

旋转矩阵和变换矩阵<B>对加法是不封闭的</B>，对于任意两个旋转矩阵$R_1,R_2$，按照矩阵加法的定义，和不再是一个旋转矩阵：

$$
R_1 + R_2 \notin SO(3), \quad T_1 + T_2 \notin SE(3). \tag{4.3}
$$

$SO(3)$和$SE(3)$关于乘法是封闭的：
$$
R_1 R_2 \in SO(3), \quad T_1 T_2 \in SES(3). \tag{4.4}
$$

也可以对任何一个旋转或变换矩阵求逆，乘法对应着旋转或变换的符合，两个旋转矩阵相乘表示做了两次旋转。

> 对于这汇总只有一个(良好的，封闭的)运算的集合，我们称之为<B>群</B>。

## 4.1.1 群

群(Group)是<B>一种集合</B>加上<B>一种运算</B>的代数结构，把集合记作$A$，运算记作$\ \cdot$，那么群可以记作$G=(A,\cdot)$。群要求这个运算满足以下几个条件：

1. 封闭性：$\forall a_1,a_2 \in A, a_1 \cdot a_2 \in A$.
2. 结合律：$\forall a_1, a_2, a_3 \in A, (a_1 \cdot a_2) \cdot a_3 = a_1 \cdot (a_2 \cdot a_3)$.
3. 幺元：$\exists a_0 \in A, \quad s.t. \quad \forall a \in A, a_0 \cdot a = a \cdot a_0 = a$.
4. 逆：$\forall a \in A, \exists a^{-1} \in A, \quad s.t. \quad a\cdot a^{-1} = a_0$.

> 可以验证，旋转矩阵集合和矩阵乘法构成群，同样，变换矩阵和矩阵乘法也构成群。

矩阵中常见的群有：

- 一般线性群GL(n) 指$n \times n$ 的可逆矩阵，它们对矩阵乘法成群。
- 特殊正交群SO(n) 也就是所谓的旋转矩阵群，其中SO(2)和SO(3)最为常见。
- 特殊欧氏群SE(n) 也就是前面提到的$n$维欧氏变换，如SE(2)和SE(3)。


> <b>李群</b>是指具有连续(光滑)性质的群，与之相反，像整数群$\mathbb{Z}$那样离散的群没有连续性质，所以不是李群。SO(n)和SE(n)在实数空间上是连续的，所以它们是李群。

## 4.1.2 李代数的引出

考虑任意旋转矩阵$R$，它满足：
$$
RR^T=I. \tag{4.5}
$$

> $R$是某个相机的旋转，它会随着时间连续地变化，即为时间的函数：$R(t)$。由于它仍是旋转矩阵，有

$$
R(t)R(t)^T = I.
$$

> 等式两边对时间求导，得到：
$$
\dot{R}(t)R(t)^T + R(t)\dot{R}(t)^T = 0.
$$

> 整理得：
$$
\dot{R}(t)R(t)^T = -\left ( \dot{R}(t)R(t)^T \right ) ^T. \tag{4.6}
$$

> 可以看出，$\dot{R}(t)R(t)^T$是一个<B>反对称</B>矩阵，在第三节引入$^\wedge$符号时，将一个向量变成了反对称矩阵。同理，对于任意反对称矩阵，我们也能找到唯一与之对应的向量。

把这个运算用符号$^\vee$表示：

$$
a^{\wedge}=A=\begin{bmatrix} 0 & -a_3 & a_2 \\ a_3 & 0 & -a_1 \\ -a_2 & a_1 & 0 \end{bmatrix}, \quad A^{\vee}=a. \tag{4.7}
$$

于是，由于$\dot{R}(t)R(t)^T$是一个反对称矩阵，可以找到一个三维向量$\phi(t) \in \mathbb{R}^3$与之对应：

$$
\dot{R}(t)R(t)^T=\phi(t)^{\wedge}.
$$

等式两边右乘$R(t)$，由于$R$为正交矩阵，有：

$$
\dot{R}(t)=\phi(t)^{\wedge}R(t)=\begin{bmatrix} 0 & -\phi_3 & \phi _2 \\ \phi_3 & 0 & -\phi_1 \\ -\phi_2 & \phi_1 & 0\end{bmatrix} R(t). \tag{4.8}
$$

<B>可以看到，每对旋转矩阵求一次导数，只需要左乘一个$\phi^{\wedge}$(t)即可</B>。

考虑$t_0=0$时，设此时旋转矩阵为$R(0)=I$。按照导数，可以把$R(t)$在$t=0$附近进行一阶泰勒展开：

$$
\begin{aligned}
R(t) &\approx R(t_0) + \dot{R}(t_0)(t-t_0) \\
&=I + \phi(t_0)^{\wedge}(t).
\end{aligned} \tag{4.9}
$$
> <B>$\phi$反映了$R$的导数性质</B>，故称它在$SO(3)$原点附近的正切空间(Tangent Space)上。同时在$t_0$附近，设$\phi$保持为常数$\phi(t_0)=\phi_0$，那么根据式(4.8)，有：

$$
\dot{R}(t)=\phi(t_0)^{\wedge}R(t)=\phi_0^{\wedge}R(t).
$$

上式是一个关于$R$的微分方程，而且有初始值$R(0)=I$，解得：

$$
R(t)=\exp(\phi_0^{\wedge}t). \tag{4.10}
$$

> 上式对微分方程和初始值均成立，这说明在$t=0$附近，旋转矩阵可以由$\exp({\phi_0^{\wedge}}t)$计算出来。
>
> 可以看到，旋转矩阵$R$与另一个反对称矩阵$\phi_0^{\wedge}t$通过指数关系发生了联系。

在这里，有两个问题需要澄清：

1. 给定某时刻的$R$，就能求得一个$\phi$，它描述了$R$在局部的导数关系。与$R$对应的$\phi$有什么含义呢？<B>$\phi$正是对应到$SO(3)$上的李代数$\mathfrak{so}(3)$</B>;
2. 给定某个向量$\phi$，矩阵指数$\exp{(\phi^{\wedge})}$如何计算？反之，给定$R$时，能否有相反的运算来计算$\phi$？事实上，这正是李群与李代数间的指数/对数映射。

## 4.1.3 李代数的定义

每个李群都有与之对应的李代数，李代数描述了李群的局部性质，准确地说，是单位元附近的正切空间。一般的李代数的定义如下：

李代数由一个集合$\mathbb{V}$、一个数域$\mathbb{F}$和一个二元运算$[,]$组成。如果它们满足以下几条性质，则称$(\mathbb{V}, \mathbb{F}, [,])$为一个李代数，记作$\mathfrak{g}$。

1. 封闭性 $\quad \forall X,Y \in \mathbb{V}, [X,Y]\in \mathbb{V}$.
2. 双线性 $\quad \forall X,Y,Z \in \mathbb{V}, \ a,b \in \mathbb{F}$，有
$$
[aX + bY, Z]=a[X,Z]+b[Y,Z], \quad [Z,aX+bY]=a[Z,X]+b[Z,Y].
$$
3. 自反性 $\quad \forall X \in \mathbb{V}, [X,X]=0$.
4. 雅可比等价 $\quad \forall X,Y,Z \in \mathbb{V}, [X,[Y,Z]] + [Z,[X,Y]] + [Y,[Z,X]] = 0$.

其中二元运算被称为<B>李括号</B>，从表面上，李代数所需要的性质还是挺多的，相比于群中的较为简单的二元运算，李括号表达了两个元素的差异。它不要求结合律，而要求元素和自己做李括号之后为零的性质。

作为例子，三维向量$\mathbb{R}^3$上定义的叉积$\times$是一种李括号，因此$\mathfrak{g}=(\mathbb{R}^3, \mathbb{R}, \times)$构成了一个李代数。

## 4.1.4 李代数$\mathfrak{so}(3)$

之前提到的$\phi$，事实上是一种李代数，$SO(3)$对应的李代数是定义在$\mathbb{R}^3$上的向量，我们记作$\phi$，根据前面的推导，每个$\phi$都可以生成一个反对称矩阵：

$$
\Phi=\phi^{\wedge}=\begin{bmatrix} 0 & -\phi_3 & \phi_2 \\ \phi_3 & 0 & -\phi_1 \\ -\phi_2 & \phi_1 & 0 \end{bmatrix} \in \mathbb{R}^{3 \times 3}. \tag{4.11}
$$

两个向量$\phi_1, \phi_2$的李括号为

$$
[\phi_1, \phi_2]=(\Phi_1 \Phi_2 - \Phi_2 \Phi_1)^{\vee}. \tag{4.12}
$$

由于向量$\phi$与反对称矩阵是一一对应的，在不引起歧义的情况下，就说$\mathfrak{so}(3)$的元素是三维向量或者三维反对称矩阵，不加区别：

$$
\mathfrak{so}(3) = \left\{ \phi \in \mathbb{R}^3, \Phi = \phi^{\wedge} \in \mathbb{R}^{3 \times 3} \right\}. \tag{4.13}
$$

总结：$\mathfrak{so}(3)$是一个由<B>三维向量</B>组成的集合，每个向量对应一个反对称矩阵，可以用于表达旋转矩阵的导数。它与SO(3)的关系由指数映射给定：

$$
R=exp(\phi^{\wedge}). \tag{4.14}
$$

## 4.1.5 李代数$\mathfrak{se}(3)$

对于SE(3)，它也有对应的李代数$\mathfrak{se}(3)$，与$\mathfrak{so}(3)$相似，$\mathfrak{se}(3)$位于$\mathbb{R}^6$空间中：

$$
\mathfrak{se}(3) = \left\{ \xi= \begin{bmatrix} \rho \\ \phi \end{bmatrix} \in \mathbb{R}^6, \rho\in\mathbb{R}^3, \phi\in\mathfrak{so}(3), \xi^{\wedge} = \begin{bmatrix} \phi^{\wedge} & \rho \\ 0^T & 0 \end{bmatrix} \in \mathbb{R}^{4 \times 4} \right\}. \tag{4.15}
$$


