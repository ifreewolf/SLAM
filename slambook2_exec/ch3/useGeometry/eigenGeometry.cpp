#include <iostream>
#include <cmath>

using namespace std;

#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace Eigen;

/**
 * 演示 Eigen 几何模块的使用方法
*/

int main(int argc, char **argv)
{
    // Eigen/Geometry 模块提供了各种旋转和平移的表示
    // 3D 旋转直接使用 Matrix3d 或 Matrix3f
    Matrix3d rotation_matrix = Matrix3d::Identity();    // 单位矩阵
    
    // 旋转向量
    // 旋转向量使用 AngleAxis，它底层不直接是Matrix，但运算可以当作矩阵（因为重载了运算符）
    // std::cout << "rotation_matrix: \n" << rotation_matrix << std::endl;
    AngleAxisd rotation_vector(M_PI / 4, Vector3d(0, 0 , 1));   // 沿 z 轴旋转 45 度
    cout.precision(3);
    cout << "rotation matrix (rotation_vector to matrix) = \n" << rotation_vector.matrix() << endl; // 用matrix()转换成矩阵

    // 也可以直接赋值
    rotation_matrix = rotation_vector.toRotationMatrix();
    cout << "rotation matrix (rotation_vector toRotationMatrix) = \n" << rotation_matrix << endl; // 用matrix()转换成矩阵
    // 用 AngleAxis 可以进行坐标变换
    Vector3d v(1, 0, 0);
    Vector3d v_rotated = rotation_vector * v;
    std::cout << "(1, 0, 0) after rotation (by angle anxis) = " << v_rotated.transpose() << std::endl;

    // 或者用旋转矩阵
    v_rotated = rotation_matrix * v;
    std::cout << "(1, 0, 0) after rotation (by matrix) = " << v_rotated.transpose() << std::endl;

    // 欧拉角：可以将旋转矩阵直接转换成欧拉角
    Vector3d euler_angles = rotation_matrix.eulerAngles(2, 1, 0);   // ZYX顺序，即yaw-pitch-roll顺序
    std::cout << "yaw pitch roll = " << euler_angles.transpose() << std::endl;

    // 欧氏变换矩阵使用 Eigen::Isometry
    Isometry3d T = Isometry3d::Identity();  // 虽然称为3d，实质上是 4*4 的矩阵
    T.rotate(rotation_vector);              // 按照rotation_vector进行旋转
    T.pretranslate(Vector3d(1, 3, 4));      // 把平移向量设成(1, 3, 4)
    std::cout << "Transform matrix = \n" << T.matrix() << std::endl;

    // 用变换矩阵进行坐标变换
    Vector3d v_transformed = T * v;         // 相当于 R*v + t
    std::cout << "v transformed = " << v_transformed.transpose() << std::endl;

    // 对于仿射和射影变换，使用Eigen::Affine3d 和 Eigen::Projective3d 即可，

    // 四元数
    // 可以直接把AngleAxis赋值给四元数，反之亦然
    Quaterniond q = Quaterniond(rotation_vector);
    std::cout << "quaternion from rotation vector = " << q.coeffs().transpose() << std::endl;   // coeffs的顺序是（x,y,z,w)，w为实部，前三者为虚部
    
    // 也可以把旋转矩阵赋给它
    q = Quaterniond(rotation_matrix);
    std::cout << "quaternion from rotation matrix = " << q.coeffs().transpose() << std::endl;

    // 使用四元数旋转一个向量，使用重载的乘法即可
    v_rotated = q * v;  // 注意数学上是qvq^{-1}
    std::cout << "(1, 0, 0) after rotation matrix = " << v_rotated.transpose() << std::endl;

    // 用常规向量乘法表示，则应该计算如下
    std::cout << "should be equal to " << (q * Quaterniond(0, 1, 0, 0) * q.inverse()).coeffs().transpose() << std::endl;

    return 0;
}