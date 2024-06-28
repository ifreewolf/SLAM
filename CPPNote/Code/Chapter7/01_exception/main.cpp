#include <iostream>

int myDiv(int a, int b)
{
    if (b == 0) {
        return -1;  // -1表示失败
    }
    return a / b;
}

void test01()
{
    int ret = myDiv(10, -10);
    if (ret == -1) {    // 返回值标识错误值与计算值冲突了
        std::cout << "程序异常" << std::endl;
    } else {
        std::cout << "程序正常" << std::endl;
    }
}

int main(int argc, char **argv)
{

    return 0;
}