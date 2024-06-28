#include <iostream>

int myDiv(int a, int b)
{
    if (b == 0) {
        throw 0;    // 抛出异常
    }
    return a / b;
}

void test01()
{
    try {
        int ret = myDiv(10, 0);
        std::cout << "ret = " << ret << std::endl;
    } catch(int e) {    // 只捕获抛出是int类型的异常
        std::cout << "捕获到int类型异常 e = " << e << std::endl;
    } catch (float e) {// 只捕获抛出是int类型的异常
        std::cout << "捕获到float类型异常 e = " << e << std::endl;
    } catch (char e) {// 只捕获抛出是int类型的异常
        std::cout << "捕获到char类型异常 e = " << e << std::endl;
    } catch (...) {
        std::cout << "捕获到其他异常" << std::endl;
    }

    std::cout << "程序做其他事情" << std::endl;
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}