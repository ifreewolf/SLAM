#include <iostream>

class Base
{
private:
    int a;
public:
    Base()
    {
        std::cout << "[父类的无参构造函数]" << std::endl;
    }
    Base(int a)
    {
        this->a = a;
        std::cout << "[父类的有参构造函数]" << std::endl;
    }
    ~Base()
    {
        std::cout << "[父类的析构函数]" << std::endl;
    }
};

class Son : public Base
{
private:
    int b;
public:
    Son()
    {
        std::cout << "[子类的无参构造函数]" << std::endl;
    }
    Son(int b)
    {
        this->b = b;
        std::cout << "[子类的有参构造函数]" << std::endl;
    }
    // 子类必须用初始化列表显式调用父类的有参构造
    Son(int a, int b) : Base(a)
    {
        this->b = b;
        std::cout << "[子类的有参构造函数 a, b]" << std::endl;
    }
    ~Son()
    {
        std::cout << "[子类的析构函数]" << std::endl;
    }
};

void test01()
{
    // 子类默认调用父类的无参构造，如果父类没有无参构造则会报错：no matching function for call to ‘Base::Base()’
    Son son(10);

    std::cout << "======================" << std::endl;
    // 子类必须用初始化列表显式调用父类的有参构造
    Son son2(10, 20);    
}

int main()
{
    test01();

    return 0;
}