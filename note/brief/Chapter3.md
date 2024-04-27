# 一. 旋转矩阵

1. 向量内积
   $$
   a \cdot b = a^T b = \sum_{i=1}^{3} a_i b_i = \lvert a \rvert \lvert b \rvert \cos  \langle a,b \rangle. \tag{3.1}
   $$
   其中$\langle a,b \rangle$指向量$a,b$的夹角。它表示一个向量在另一个向量方向上的投影的长度乘以第二个向量的长度。简而言之，向量内积就是两个向量在同一个方向上的“长度”的乘积。

2. 向量外积
   $$
   a \times b = \begin{Vmatrix}
    e_1 & e_2 & e_3 \\
    a_1 & a_2 & a_3 \\
    b_1 & b_2 & b_3
   \end{Vmatrix} = \begin{bmatrix}
    a_2b_3 - a_3b_2 \\
    a_3b_1 - a_1b_3 \\
    a_1b_2 - a_2b_1
   \end{bmatrix} = \begin{bmatrix}
    0 & -a_3 & a_2 \\
    a_3 & 0 & -a_1 \\
    -a_2 & a_1 & 0
   \end{bmatrix}b \overset{def}{=} a^{\wedge}b. \tag{3.2}
   $$

3. 欧式变换
   坐标系间的转换就是<b>欧式变换</b>，欧式变换由旋转和平移组成。
   单位正交基$(e_1,e_2,e_3)$经过一次旋转变成了$(e_1',e_2',e_3')$，对于向量$a$在世界坐标系下是不动的。因此有：
   $$
   [e_1,e_2.e_3]\begin{bmatrix}
    a_1 \\ a_2 \\ a_3
   \end{bmatrix} = [e_1',e_2',e_3']\begin{bmatrix}
    a_1' \\ a_2' \\ a_3'
   \end{bmatrix}. \tag{3.3}
   $$
   同时左乘$[e_1^T,e_2^T,e_3^T]^T$，那么左边的系数就变成了单位矩阵，所以：
   $$
   \begin{bmatrix}
    a_1 \\ a_2 \\ a_3
   \end{bmatrix} = \begin{bmatrix}
    e_1^Te_1' & e_1^Te_2' & e_1^Te_3' \\
    e_2^Te_1' & e_2^Te_2' & e_2^Te_3' \\
    e_3^Te_1' & e_3^Te_2' & e_3^Te_3'
   \end{bmatrix} \begin{bmatrix}
    a_1' \\ a_2' \\ a_3'
   \end{bmatrix} \overset{def}{=} Ra'. \tag{3.4}
   $$
   $\mathbin{R}$称为旋转矩阵，该矩阵各分量是两个坐标系基的内积，由于基向量的长度为1，所以<b>旋转矩阵各分量其实是各基向量夹角的余弦值</b>，该矩阵也叫<b>方向余弦矩阵</b>(Direction Cosine Matrix)。

4. 旋转矩阵
   旋转矩阵是一个行列式为1的正交矩阵，反之，行列式为1的正交矩阵也是一个旋转矩阵。将$n$维旋转矩阵的集合定义如下：
   $$
   \mathrm{SO}(n)=\left\{ R \in \mathbb{R}^{n\times n}|RR^T=I,det(R)=1 \right\}. \tag{3.5}
   $$
   $\mathrm{SO}(n)$是<B>特殊正交群</B>(Special Orthogonal Group)。特别地，$\mathrm{SO}(3)$就是指三维空间的旋转，通过旋转矩阵，可以直接谈论两个坐标系之间的旋转变换，而不用再从基开始谈起。
   旋转矩阵为正交矩阵，它的逆（即转置）描述了一个相反的旋转。意味着：
   $$
   a = R^{-1}a' = R^Ta'. \tag{3.5}
   $$
   $R^{T}$描述了一个相反的旋转。

5. 欧式变换的平移
   平移是一个向量，向量$a$经过一次欧式变换后，有
   $$
   a' = Ra + t. \tag{3.6}
   $$

   定义坐标系1、坐标系2，向量a在两个坐标系下的坐标为$a_1,a_2$，它们之间的关系是：
   $$
   a_1 = R_{12}a_2 + t_{12}. \tag{3.7}
   $$
   $R_{12}$是指“把坐标系2的向量变换到坐标系1”中，由于向量乘在这个矩阵的右边，它的下标是<b>从右读到左的</b>。$t_{12}$对应的是坐标系1原点指向坐标系2原点的向量，<b>在坐标系1下取的坐标</b>，读作“从1到2的向量”。

6. 变换矩阵与齐次坐标
   假设进行了两次变换：$R_1,t_1$和$R_2,t_2$:
   $$
   b = R_1 a + t, c = R_2 b + t_2.
   $$
   那么从$a$到$c$的变换为
   $$
   c = R_2(R_1a + t_1) + t_2.
   $$
   这样的形式在变换多次形式就很麻烦，引入<b>齐次坐标</b>和变换矩阵，重写式子：
   $$
   \begin{bmatrix}
    a; \\ 1
   \end{bmatrix} = \begin{bmatrix}
    R & t \\ 0^T & 1
   \end{bmatrix} \begin{bmatrix}
    a \\ 1
   \end{bmatrix} \overset{def}{=} T\begin{bmatrix}
    a \\ 1
   \end{bmatrix}. \tag{3.8}
   $$
   矩阵$T$称为<b>变换矩阵(Transform Matrix)</b>。
   用$\tilde{a}$表示$a$的齐次坐标，依靠齐次坐标和变换矩阵，两次变换的叠加就可以：
   $$
   \tilde{b}=T_1 \tilde{a}, \tilde{c}=T_2 \tilde{b} \Rightarrow \tilde{c}=T_2 T_1 \tilde{a}. \tag{3.9}
   $$
   为了表示方便，在不引起歧义的情况下，直接把它写成$b=Ta$，默认进行了齐次坐标的转换。

7. 特殊欧式群
   变换矩阵$T$，特殊结构：左上角为旋转矩阵，右侧为平移向量，左下角为0向量，右下角为1。这种矩阵又称为<b>特殊欧氏群(Special Euclidean Group)</b>:
   $$
   \mathrm{SE}(3) = \left\{ T = \begin{bmatrix}
    R & t \\ 0^T & 1
   \end{bmatrix} \in \mathbb{R}^{4\times4}| R\in \mathrm{SO}(3). t \in \mathbb{R}^3 \right\}. \tag{3.10}
   $$
   与$\mathrm{SO}(3)$一样，求解该矩阵的逆表示一个反向的变换：
   $$
   T^{-1}=\begin{bmatrix}
    R^T & -R^T t \\ 0^T & 1
   \end{bmatrix}. \tag{3.11}
   $$
   用$T_{12}$表示从2到1的变换。


---

# 二.旋转向量和欧拉角

1. 旋转向量
   矩阵方式存在以下两个缺点：
   1) SO(3)的旋转矩阵有9个量，但一次旋转只有3个自由度。表达方式是冗余的，同理，变换矩阵用16个量表达了6自由度。是否有更紧凑的表示？
   2) 旋转矩阵自身带有约束：正交矩阵，行列式为1。当估计或优化旋转矩阵时，这些约束会使得求解变得更困难。
  
   旋转向量(轴角，Axis-Angle)：任意旋转都可以用<b>一个旋转轴和一个旋转角</b>来刻画，旋转向量其方向和旋转轴一致，而长度等于旋转角。
   对于变换矩阵，使用一个旋转向量和一个平移向量即可表达一次变换，这时的变量的维数刚好是6维。

   旋转矩阵$R$，假设旋转轴为一个单位长度的向量$n$，角度为$\theta$，那么向量$\theta n$可以描述这个旋转。从旋转向量到旋转矩阵的转换过程由<b>罗德里格斯公式</b>(Rodrigues's Formula)表明：
   $$
   R = \cos \theta I + (1 - \cos \theta)nn^T + \sin \theta n^{\wedge}. \tag{3.12}
   $$
   反之，从旋转矩阵到旋转向量的转换，对于转角$\theta$，取两边的<b>迹</b>，有：
   $$
   \begin{aligned}
    tr(R) &= \cos\theta \mathrm{tr}(I) + (1 - \cos\theta)\mathrm{tr}(nn^T) + \sin\theta \mathrm{tr}(n^{\wedge}) \\
    &= 3\cos\theta + (1 - \cos\theta) \\
    &= 1 + 2\cos\theta
   \end{aligned}. \tag{3.13}
   $$
   因此：
   $$
   \theta = \arccos\frac{\mathrm{tr}(R) - 1}{2}. \tag{3.14}
   $$
   关于转轴$n$，旋转轴上的向量在旋转之后不发生改变，说明：
   $$
   Rn = n. \tag{3.15}
   $$
   转轴$n$是矩阵$R$特征值1对应的特征向量。

2. 欧拉角
   旋转矩阵和旋转向量对人类来说都非常不直观，欧拉角使用<b>3个分离的转角</b>，把一个旋转分解成3次绕不同轴的旋转。
   欧拉角的一个重大的缺点是会碰到著名的<b>万向锁问题</b>(Gimbal Lock)：在俯仰角为$\pm90\degree$时，第一次旋转与第三次旋转将使用同一个轴，使得系统丢失一个自由度。被称为奇异性问题。
   理论上只要想用3个实数来表达三维旋转，都会不可避免地碰到奇异性问题，由于这种原理，欧拉角不适用于插值和迭代，往往只用于人机交互中。

---

# 三. 四元数

旋转矩阵具有冗余性，欧拉角和旋转向量紧凑，但具有奇异性。事实上，<b>找不到不带奇异性的三维向量描述方式</b>。
四元数(Quaternion)是一种扩展的复数，<b>既紧凑又没有奇异性</b>，缺点是不够直观，一个四元数$q$拥有一个实部和三个虚部：

$$
q = q_0 + q_1 \mathrm{i} + q_2 \mathrm{j} + q_3 \mathrm{k}, \tag{3.16}
$$

$\mathrm{i},\mathrm{j},\mathrm{k}$是四元数的三个虚部，这三个虚部满足以下关系式：

$$
\left\{
  \begin{array}{l}
    i^2 = j^2 = k^2 = -1 \\
    ij = k, ji = -k \\
    jk = i, kj = -i \\
    ki = j, ik = -j
  \end{array}
\right. . \tag{3.17}
$$

也有更简单的表达方式(一个标量和一个向量)：

$$
q = [s, v]^T, s =q_0\in\mathbb{R}, v = [q_1, q_2, q_3]^T \in \mathbb{R}^3.
$$

$s$称为四元数的实部，而$v$称为它的虚部，如果一个四元数的虚部为0，则称为<b>实四元数</b>；反之，若它的实部为0，则称为<b>虚四元数</b>。
可以用<b>单位四元数</b>表示三维空间中任意一个旋转。

1. 四元数的运算
   $$
   q_a = s_a + x_a i + y_a j + z_a k, \quad q_b = s_b + x_b i + y_b j + z_b k.
   $$
   1) 加法和减法
      $$
      q_a \pm q_b = [s_a \pm s_b, v_a \pm v_b]^T. \tag{3.18}
      $$

   2) 乘法
      $$
      \begin{aligned}
        q_a q_b =& s_a s_b - x_a x_b - y_a y_b - z_a z_b \\
        &+(s_a x_b + x_a s_b + y_a z_a - z_a y_b)i \\
        &+(s_a y_b - x_a z_b + y_a s_b + z_a x_b)j \\
        &+(s_a z_b + x_a y_b - y_a x_b + z_a s_b)k.
      \end{aligned} \tag{3.19}
      $$
      等价于
      $$
      q_a q_b = [s_a s_b - v_a^T v_b, s_a v_b + s_b v_a + v_a \times v_b]^T. \tag{3.20}
      $$ 
   3) 模长
      $$
      \lVert q_a \rVert = \sqrt{s_a^2 + x_a^2 + y_a^2 + z_z^2}. \tag{3.21}
      $$
      两个四元数乘积的模即模的乘积，这使得单位四元数相乘后仍是单位四元数。
      $$
      \lVert q_a q_b \rVert = \lVert q_a \rVert \lVert q_b \rVert. \tag{3.22}
      $$

   4) 共轭
      $$
      q_a^* = s_a - x_a i - y_a j - z_a k = [s_a, -v_a]^T. \tag{3.23}
      $$
      四元数共轭与其本身相乘，会得到一个实四元数，其实部为模长的平方：
      $$
      q^* q = q q^* = [s_a^2 + v^Tv, 0]^T. \tag{3.24}
      $$

   5) 逆
      $$
      q^{-1} = q^* / \lVert q \rVert^2. \tag{3.25}
      $$
      四元数和自己的逆乘积为实四元数1：
      $$
      qq^{-1} = q^{-1}q = 1. \tag{3.26}
      $$
      如果$q$为单位四元数，其逆和共轭就是同一个量，同时，乘积的逆具有和矩阵相似的性质：
      $$
      (q_a q_b)^{-1} = q_b^{-1} q_a^{-1}. \tag{3.27}
      $$

   6) 数乘
      $$
      k\mathbb{q} = [ks, kv]^T. \tag{3.28}
      $$

2. 用四元数表示旋转
   空间三维点$p=[x,y,z]\in \mathbb{R}^3$，单位四元数$q$指定的旋转，三维点$p$经过旋转之后变为$p'$，如果使用矩阵描述，那么有$p' = Rp$。使用四元数描述：
   首先把三维空间点用一个虚四元数来描述：
   $$
   p = [0, x, y, z]^T = [0, v]^T.
   $$
   相当于把四元数的3个虚部与空间中的3个轴相对应，那么，旋转后的点$p'$可表示为这样的乘积：
   $$
   p' = qpq^{-1}. \tag{3.29}
   $$
   这里的乘积是四元数乘法，结果也是四元数，最后把$p'$的虚部取出，就是旋转之后点的坐标。
   
3. 四元数到其他旋转表示的转换
   四元数乘法也可以写成一种矩阵的乘法，设$q=[s,v]^T$，那么定义如下的符号$^{+}$和$^{\oplus}$为
   $$
   q^{+} = \begin{bmatrix}
    s & -v^T \\ v & sI + v^{\wedge}
   \end{bmatrix}, \quad q^{\oplus} = \begin{bmatrix}
      s & -v^T \\ v & sI - v^{\vee}
    \end{bmatrix}. \tag{3.30}
   $$

   这两个符号将四元数映射成一个$4\times 4$的矩阵，于是四元数乘法可以写成矩阵的形式：
   $$
   q_1^{+} q_2 = \begin{bmatrix}
    s_1 & -v^T \\ v_1 & s_1 I + v_1^{\wedge}
   \end{bmatrix} \begin{bmatrix}
    s_2 \\ v_2
   \end{bmatrix} = \begin{bmatrix}
    -v_1^Tv_2 + s_1 s_2 \\ s_1 v_2 + s_2 v_1 + v_1^{\wedge}v_2
   \end{bmatrix} = q_1 q_2. \tag{3.31}
   $$
   同理：
   $$
   q_1 q_2 = q_1^+ q_2 = q_2^{\oplus}q_1. \tag{3.32}
   $$

   四元数对空间点的旋转：
   $$
   \begin{aligned}
    p' &= qpq^{-1} = q^+ p^+ q^{-1} \\
    &= q^+ q^{-1 \oplus} p. 
   \end{aligned} \tag{3.33}
   $$

   代入两个符号对应的矩阵，得
   $$
   q^+(q^{-1})^{\oplus} = \begin{bmatrix}
    s & -v^T \\ v & sI + v^{\wedge}
   \end{bmatrix} \begin{bmatrix}
    s & v^T \\ -v & sI + v^{\wedge}
   \end{bmatrix} = \begin{bmatrix}
    1 & 0 \\ 0^T & vv^T + s^2 I + 2sv^{\wedge} + (v^{\wedge})^2
   \end{bmatrix}. \tag{3.34}
   $$
   因为$p'$和$p$都是虚四元数，所以该矩阵的右下角即给出了<b>从四元数到旋转矩阵</b>的变换关系：
   $$
   R = vv^T + s^2 I + 2sv^{\wedge} + (v^{\wedge})^2. \tag{3.35}
   $$
   为了得到四元数到旋转向量的转换公式，对上式两侧求迹，得
   $$
   \begin{aligned}
    \mathrm{tr}(R) &= \mathrm{tr}(vv^T) + 3s^2 + 2s\cdot 0 + \mathrm{tr}((v^{\wedge})^2) \\
    &= v_1^2 + v_2^2 + v_3^2 + 3s^2 - 2(v_1^2 + v_2^2 + v_3^2) \\
    &= (1 - s^2) + 3s^2 - 2(1 - s^2) \\
    &= 4s^2 - 1.
   \end{aligned} \tag{3.36}
   $$
   由式子(3.14)得
   $$
   \begin{aligned}
    \theta &= \arccos \frac{\mathrm{tr}(R - 1)}{2} \\
    &= \arccos (2s^2 -  1).
   \end{aligned} \tag{3.37}
   $$
   即
   $$
    \cos \theta = 2s^2 - 1 = 2\cos^2 \frac{\theta}{2} - 1, \tag{3.38}
   $$

   所以：
   $$
   \theta = 2\arccos s. \tag{3.39}
   $$
   四元数到旋转向量的转换公式如下：
   $$
   \left\{
   \begin{array}{l}
      \theta = 2\arccos q_0 \\
      [n_x, n_y, n_z]^T = [q_1, q_2, q_3]^T / \sin \frac{\theta}{2}
   \end{array}
   \right. . \tag{3.40}
   $$
