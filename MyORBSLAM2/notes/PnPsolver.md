# EPnP算法求解

- 在ORBSLAM2中使用opencv求解矩阵Mx=0的解空间时，直接使用svd分解即可得到M矩阵的右奇异向量，为什么作者在计算时，首先使用cvMulTransposed获得M^TM矩阵，再对M^TM使用cvSVD进行计算，获得特征向量，为什么不可以直接使用cvSVD对M矩阵进行计算？
- 在 ORB-SLAM2 中，作者选择先计算𝑀𝑇𝑀矩阵再进行 SVD 分解，而不是直接对𝑀进行 SVD 分解，主要是出于以下原因：
- 1. ‌数值稳定性与计算效率‌
-   数值稳定性‌：对于高维矩阵 𝑀，直接对 𝑀 进行 SVD 分解可能导致数值不稳定，尤其是当 𝑀 的列数远大于行数时（即𝑚≪𝑛）。通过计算 𝑀𝑇𝑀，可以将问题转化为对一个 m×m 的矩阵进行 SVD，这在数值更为稳定。
-   计算效率‌：MTM 的计算量为O(mn)，而直接对 M 进行 SVD 的计算量为 O(mn+n)。对于 m≪n 的情况，前者更高效。
- 2. ‌零空间的直接提取‌
-   零空间特征‌：MTM 的零空间（即 Mx=0 的解）与 M 的零空间（即 Mx=0 的解）是正交的。通过 MTM 的 SVD，可以直接提取其零空间的特征向量，这些向量即为 M 的零空间基向量。
- 3. ‌代码实现的规范性‌
-   OpenCV API 规范‌：OpenCV 的 cvSVD 函数在处理非方阵时，通常要求输入矩阵为方阵。通过 MTM 转化为方阵，符合 OpenCV 的 API 规范。
4. ‌历史与兼容性‌
-   算法历史‌：ORB-SLAM2 的初始化算法基于早期计算机视觉文献（如），这些文献中常用 MTM 的 SVD 来处理零空间问题。保持这一传统做法有助于代码的可读性和兼容性。

> 总结‌：通过 MTM 的 SVD 分解，不仅提高了数值稳定性，还直接提取了零空间特征向量，符合 ORB-SLAM2 的算法设计需求。



根据前面函数 gauss_newton 中的一些工作,可以发现这里的系数矩阵其实就是目标函数雅克比的转置. 原目标函数:

$$ f(\mathbf{\beta})=\sum_{(i,j \  s.t. \  i<j)} \left( ||\mathbf{c}^c_i-\mathbf{c}^c_j ||^2 - ||\mathbf{c}^w_i-\mathbf{c}^w_j ||^2 \right)  $$
然后观察一下每一项的组成: $$ ||\mathbf{c}^c_i-\mathbf{c}^c_j ||^2  $$ 由论文中式12可以发现就对应着矩阵 L 中的一行,
同样地对于 $$ ||\mathbf{c}^w_i-\mathbf{c}^w_j ||^2  $$ 则对应着式(13)中向量 $$  \mathbf{\rho} $$ 的每一行.所以目标函数完全可以写成矩阵的形式:
$$ f(\mathbf{\beta})=\mathbf{L}\mathbf{\bar{\beta}}-\mathbf{\rho}  $$
注意这里使用的符号:
$$ \mathbf{\bar{\beta}}= \begin{bmatrix} \beta_{11}\\\beta_{12}\\\beta_{22}\\\beta_{13}\\\beta_{23}\\\beta_{33}\\\beta_{14}\\\beta_{24}\\\beta_{34}\\\beta_{44} \end{bmatrix} $$
为了方便求导,计算得到一个中间结果先:

$$
    \begin{split}
        \mathbf{\bar{L}}
        &=
        \mathbf{L}\mathbf{\bar{\beta}} \\
        &=
        \begin{bmatrix}
            L_{11}\beta_{11}+L_{12}\beta_{12}+L_{13}\beta_{22}+\cdots+L_{1A}\beta_{44} \\
            L_{21}\beta_{11}+L_{22}\beta_{22}+L_{13}\beta_{22}+\cdots+L_{2A}\beta_{44} \\
            \cdots \\
            L_{61}\beta_{11}+L_{62}\beta_{22}+L_{63}\beta_{22}+\cdots+L_{6A}\beta_{44}
        \end{bmatrix}
        =
        \begin{bmatrix}
            \mathbf{L_1} \\
            \mathbf{L_2} \\
            \cdots \\
            \mathbf{L_6}
        \end{bmatrix}
    \end{split} 
$$
 然后原来的目标函数矩阵表示变成为了:
 $$  f(\mathbf{\beta})=\mathbf{\bar{L}}-\mathbf{\rho} $$
 接下来准备求目标函数的雅克比.注意到只有矩阵 $ \mathbf{\bar{L}} $ 和优化变量 $ \mathbf{\beta} $ 有关系,因此有:
$$ \begin{split}
\frac{\partial f(\mathbf{\beta})}{\partial \mathbf{\beta}}&=\frac{\partial \mathbf{L}}{\partial \mathbf{\beta}}\\
&=
\begin{bmatrix}
\frac{\partial \mathbf{L}}{\partial \beta_1}&\frac{\partial \mathbf{L}}{\partial \beta_2}&
\frac{\partial \mathbf{L}}{\partial \beta_3}&\frac{\partial \mathbf{L}}{\partial \beta_4}
\end{bmatrix} \\
&=
\begin{bmatrix}
\frac{\partial \mathbf{L}_1}{\partial \beta_1}&\frac{\partial \mathbf{L}_1}{\partial \beta_2}&
\frac{\partial \mathbf{L}_1}{\partial \beta_3}&\frac{\partial \mathbf{L}_1}{\partial \beta_4}\\
\frac{\partial \mathbf{L}_2}{\partial \beta_1}&\frac{\partial \mathbf{L}_2}{\partial \beta_2}&
\frac{\partial \mathbf{L}_2}{\partial \beta_3}&\frac{\partial \mathbf{L}_2}{\partial \beta_4}\\
\cdots&\cdots&\cdots&\cdots\\
\frac{\partial \mathbf{L}_6}{\partial \beta_1}&\frac{\partial \mathbf{L}_6}{\partial \beta_2}&
\frac{\partial \mathbf{L}_6}{\partial \beta_3}&\frac{\partial \mathbf{L}_6}{\partial \beta_4}
\end{bmatrix}
\end{split} $$
从优化目标函数的概念触发,其中的每一行的约束均由一对点来提供,因此不同行之间其实并无关系,可以相互独立地计算,因此对于其中的每一行:(以第一行为例)
$$ \mathbf{L}_1=
\beta_{11}L_{11}+\beta_{12}L_{12}+\beta_{22}L_{13}+\beta_{13}L_{14}+\beta_{23}L_{15}+
\beta_{33}L_{16}+\beta_{14}L_{17}+\beta_{24}L_{18}+\beta_{34}L_{19}+\beta_{44}L_{1A} $$
分别对beat进行求导:(注意为了方便这里把L的下标从1开始变成了从0开始)
$$ \frac{\partial \mathbf{L}_1}{\partial \beta_1}=2\beta_1L_{10}+\beta_2L_{11}+\beta_3L_{13}+\beta_4L_{16} \\
\frac{\partial \mathbf{L}_1}{\partial \beta_2}=\beta_1L_{11}+2\beta_2L_{12}+\beta_3L_{14}+\beta_4L_{17} \\
\frac{\partial \mathbf{L}_1}{\partial \beta_3}=\beta_1L_{13}+\beta_2L_{14}+2\beta_3L_{15}+\beta_4L_{18} \\
\frac{\partial \mathbf{L}_1}{\partial \beta_4}=\beta_1L_{16}+\beta_2L_{17}+\beta_3L_{18}+2\beta_4L_{19}  $$
就是下面计算每一行的雅克比的式子.

另外对于当前行的非齐次项, 在 gauss_newton 中简化后得到的结果为 -f(x), 也就是:
$$ ||\mathbf{c}^w_i-\mathbf{c}^w_j ||^2 - ||\mathbf{c}^c_i-\mathbf{c}^c_j ||^2 $$
每一行都会有一个特定的i和j.上式中的前者可以直接由 $ \mathbf{\rho} $ 的对应行给定,而后者则要根据论文公式(12)给出了:
$$ ||\mathbf{c}^c_i-\mathbf{c}^c_j ||^2 = \mathbf{L}_k\mathbf{\bar{\beta}} $$
这个也就是非齐次项部分的计算过程
