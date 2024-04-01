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
    Matrix<double, 3, 1> vd_3d;

    Matrix3d matrix_33 = Matrix3d::Zero();  // 初始化为零
    return 0;
}