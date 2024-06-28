#include <iostream>


class MyException
{
public:
    MyException()
    {
        std::cout << "异常构造" << std::endl;
    }
    MyException(const MyException &ob)
    {
        std::cout << "异常拷贝构造" << std::endl;
    }
    ~MyException()
    {
        std::cout << "异常的析构" << std::endl;
    }
};

void test01()
{
    try {
        // MyException ob; // 第一个构造
        // throw ob;       // 抛出的时候，把这个对象拷贝到一个临时区域，发生了一次拷贝构造
        // 离开这个作用区域之后，第一个构造需要析构

        // throw new MyException;

        throw MyException();
    }
    catch (MyException &e) {
        std::cout << "捕获到MyException&异常" << std::endl;
    }
    catch (MyException e) {   // 临时对象赋给形参，又发生了一次拷贝。
        std::cout << "捕获到MyException异常" << std::endl;
    }   // 离开这个区域之后，发生了两次析构
    catch (MyException *e) {
        std::cout << "捕获到MyException*异常" << std::endl;
        delete e;
    }
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}