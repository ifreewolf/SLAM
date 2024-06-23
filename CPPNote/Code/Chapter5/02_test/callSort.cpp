#include <iostream>

class Other
{
public:
    Other()
    {
        std::cout << "对象成员的构造函数" << std::endl; 
    }
    ~Other()
    {
        std::cout << "对象成员的析构函数" << std::endl;
    }
};

class Base
{
public:
    Base()
    {
        std::cout << "父类的无参构造函数" << std::endl;
    }
    ~Base()
    {
        std::cout << "父类的析构函数" << std::endl;
    }
};

class Son : public Base
{
public:
    Son()
    {
        std::cout << "子类的构造函数" << std::endl;
    }
    ~Son()
    {
        std::cout << "子类的析构函数" << std::endl;
    }

    Other other;    // 对象成员
};

int main(int argc, char **argv)
{
    Son son;
    return 0;
}