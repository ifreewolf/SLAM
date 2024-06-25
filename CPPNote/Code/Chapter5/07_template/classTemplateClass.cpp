#include <iostream>
#include <string>

template<class T>
class Base
{
private:
    T num;
public:
    Base(T num)
    {
        std::cout << "Base有参构造" << std::endl;
        this->num = num;
    }
    ~Base()
    {
        std::cout << "Base析构函数" << std::endl;
    }
    void showNum(void)
    {
        std::cout << "Base num = " << this->num << std::endl;
    }
};

// 类模板派生普通类，必须给基类指定类型
class Son1 : public Base<int>
{
public:
    // 基类构造函数初始化时，也必须指定类型
    Son1(int a) : Base<int>(a)
    {
        std::cout << "Son1有参构造" << std::endl;
    }
    ~Son1()
    {
        std::cout << "Son1析构函数" << std::endl;
    }
};

class Son2 : public Base<std::string>
{
public:
    Son2(std::string str) : Base<std::string>(str)
    {
        std::cout << "Son2有参构造" << std::endl;
    }
    ~Son2()
    {
        std::cout << "Son2析构函数" << std::endl;
    }
};

void test01()
{
    Son1 ob1(100);
    ob1.showNum();

    Son2 ob2("son2");
    ob2.showNum();
}

int main(int argc, char **argv)
{
    test01();
    return 0;
}