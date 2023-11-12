#include <iostream>
#include <cmath>

using namespace std;

#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace Eigen;

int main(int argc, char **argv)
{
    Matrix3d rotation_matrix = Matrix3d::Identity();
    AngleAxisd rotation_vector(M_PI / 4, Vector3d(0, 0, 1)); // 沿 Z 轴旋转 45 度
    cout.precision(3);
    cout << "rotation matrix = \n" << rotation_vector.matrix() << endl; // 用matrix()转换成矩阵

    rotation_matrix = rotation_vector.toRotationMatrix();
    cout << "rotation_vector to rotation_matrix = \n" << rotation_matrix << endl;

    // 用AngleAxisd 可以进行坐标变换
    Vector3d v(1, 0, 0);
    Vector3d v_rotated = rotation_vector * v;
    cout << "(1,0,0) after rotation (by angle axis) = " << v_rotated.transpose() << endl;
    // 用旋转矩阵进行旋转
    v_rotated = rotation_matrix * v;
    cout << "(1,0,0) after rotation (by matrix) = " << v_rotated.transpose() << endl;

    // 欧拉角
    // 旋转矩阵直接转换成欧拉角
    Vector3d euler_angle = rotation_matrix.eulerAngles(2, 1, 0); // ZYX顺序， 即yaw-pitch-roll顺序
    cout << "yaw pitch roll = " << euler_angle.transpose() << endl;

    // 欧式变换矩阵使用 Eigen::Isometry
    Isometry3d T = Isometry3d::Identity();  // 虽然称为 3d, 实质上是 4*4 的矩阵
    T.rotate(rotation_vector);              // 按照 rotation_vector 进行旋转
    T.pretranslate(Vector3d(1, 3, 4));      // 把平移变量设成 (1,3,4)
    cout << "Transform matrix = \n" << T.matrix() << endl; // matrix()转换成矩阵

    // 用变换矩阵进行坐标变换
    Vector3d v_transformed = T * v;
    cout << "v transformed = " << v_transformed.transpose() << endl;

    // 四元数
    // 可以直接把AngleAxis赋值给四元数， 反之亦然 --旋转向量
    Quaterniond q = Quaterniond(rotation_vector);
    cout << "quaternion from rotation vector = " << q.coeffs().transpose() << endl; // coeffs的顺序是 (x,y,z,w)， w为是不， 前三者为虚部

    // 旋转矩阵
    q =  Quaterniond(rotation_matrix);
    cout << "quaternion from rotation matrix = " << q.coeffs().transpose() << endl;

    // 使用 四元数 旋转一个向量，使用重载的乘法即可
    v_rotated = q * v; // 在数学上是 qvq^{-1}
    cout << "(1, 0, 0) after rotation (by quaternion) = " << v_rotated.transpose() << endl;

    // 用常规向量乘法表示， 则应该如下计算
    cout << "should be equal to " << (q * Quaterniond(0, 1, 0, 0) * q.inverse()).coeffs().transpose() << endl;

    return 0;
}