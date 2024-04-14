#include <iostream>

using namespace std;

#include <ctime>
// Eigen 核心部分
#include <Eigen/Core>
// 稠密矩阵的代数运算
#include <Eigen/Dense>

using namespace Eigen;

#define MATRIX_SIZE 50

int main(int argc, char **argv)
{
    // Eigen是一个模板类，前三个参数为：数据类型， 行，列
    Matrix<float, 2, 3> matrix_23;
    
    Vector3d v_3d;
    Matrix<float, 3, 1> vd_3d;

    Matrix3d matrix_33 = Matrix3d::Zero();  // 初始化为零

    // 动态大小的矩阵
    Matrix<double, Dynamic, Dynamic> matrix_dynamic;
    MatrixXd matrix_x;

    matrix_23 << 1, 2, 3, 4, 5, 6;
    std::cout << "matrix 2x3 from 1 to 6: \n" << matrix_23 << std::endl;

    // 用()访问矩阵中的元素
    std::cout << "print matrix 2x3: " << std::endl;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            std::cout << matrix_23(i, j) << "\t";
        }
        std::cout << std::endl;
    }

    // 矩阵和向量相乘(实际上仍然是矩阵和矩阵)
    v_3d << 3, 2, 1;
    vd_3d << 4, 5, 6;

    // 在Eigen里，不同类型的矩阵不能进行操作
    // Matrix<double, 2, 1> result_wrong_type = matrix_23 * v_3d;
    // 需要显式转换
    Matrix<double, 2, 1> result = matrix_23.cast<double>() * v_3d;
    std::cout << "[1,2,3;4,5,6]*[3,2,1] = " << result.transpose() << std::endl;

    Matrix<float, 2, 1> result2 = matrix_23 * vd_3d;    // matrix_23和vd_3d都是float类型的
    std::cout << "[1,2,3;4,5,6]*[4,5,6] = " << result2.transpose() << std::endl;

    // 不能搞错矩阵的维度
    // Matrix<double, 2, 3> result_wrong_dimension = matrix_23.cast<double>() * v_3d;  //  YOU_MIXED_MATRICES_OF_DIFFERENT_SIZES，编译期不通过

    // 矩阵运算
    // 四则运算：直接用+ - * / 即可
    matrix_33 = Matrix3d::Random(); // 随机数矩阵，Matrix3d和变量的类型有关
    std::cout << "random matrix: \n" << matrix_33 << std::endl;
    std::cout << "transpose: \n" << matrix_33.transpose() << std::endl; // 转置
    std::cout << "sum: " << matrix_33.sum() << std::endl;               // 元素和
    std::cout << "trace: " << matrix_33.trace() << std::endl;           // 迹
    std::cout << "times 10: \n" << 10 * matrix_33 << std::endl;         // 数乘
    std::cout << "inverse: \n" << matrix_33.inverse() << std::endl;     // 逆
    std::cout << "det: " << matrix_33.determinant() << std::endl;       // 行列式

    // 特征值
    // 实对称矩阵可以保证对角化成功
    SelfAdjointEigenSolver<Matrix3d> eigen_solver(matrix_33.transpose() * matrix_33);
    std::cout << "Eigen values = \n" << eigen_solver.eigenvalues() << std::endl;
    std::cout << "Eigen vectors = \n" << eigen_solver.eigenvectors() << std::endl;

    // 解方程
    // 求解 matrix_NN * x = v_Nd 这个方程
    // N的大小在前边宏定义，它由随机数生成
    // 直接求逆自然是最直接的，但是求逆运算量大

    Matrix<double, MATRIX_SIZE, MATRIX_SIZE> matrix_NN
        = MatrixXd::Random(MATRIX_SIZE, MATRIX_SIZE);
    matrix_NN = matrix_NN * matrix_NN.transpose(); // 保证半正定
    Matrix<double, MATRIX_SIZE, 1> v_Nd = MatrixXd::Random(MATRIX_SIZE, 1);

    clock_t time_stt = clock(); // 计时
    // 直接求逆
    Matrix<double, MATRIX_SIZE, 1> x = matrix_NN.inverse() * v_Nd;
    std::cout << "time of normal inverse is "
              << 1000 * (clock() - time_stt) / (double) CLOCKS_PER_SEC << "ms" << std::endl;
    std::cout << "x = " << x.transpose() << std::endl;

    // 对于正定矩阵，还可以用cholesky分解来解方程
    time_stt = clock();
    x = matrix_NN.colPivHouseholderQr().solve(v_Nd);
    std::cout << "time of Qr decomposition is "
              << 1000 * (clock() - time_stt) / (double) CLOCKS_PER_SEC << "ms" << std::endl;
    std::cout << "x = " << x.transpose() << std::endl;

    return 0;
}