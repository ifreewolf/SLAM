#include <iostream>

class Base
{
public:
    static int num; // 静态成员属于类，而不属于对象
    static int data;
    static void showData(void);
};
int Base::num = 100;
int Base::data = 200;

class Son : public Base
{
public:
    static int data;    // 父类和子类静态成员同名
    static void showData(void);
};
int Son::data = 300;

void Base::showData(void)
{
    std::cout << "父类的showData" << std::endl;
}

void Son::showData(void)
{
    std::cout << "子类的showData" << std::endl;
}

void test01()
{
    // 从Base类中访问
    Son::num = 150;
    std::cout << Base::num << std::endl;    // 100

    // Son类也拥有了静态成员
    std::cout << Son::num << std::endl; // 100

    // 父类和子类静态成员同名，访问子类中的成员变量
    std::cout << Son::data << std::endl;

    // 父类和子类静态成员同名，访问父类中的成员变量
    std::cout << Son::Base::data << std::endl;

    // 父类和子类，同名静态成员函数，通过子类作用域默认访问子类的静态成员函数。
    Son::showData();
     // 父类和子类，同名静态成员函数，访问父类的成员函数。
    Son::Base::showData();

}

int main(int argc, char **argv)
{
    test01();

    return 0;
}