#include <iostream>

class Base
{
    // 父类的私有数据，一旦涉及继承，在子类中不可见
public:
    int num;
public:
    Base(int num)
    {
        this->num = num;
        std::cout << "Base有参构造" << std::endl;
    }
    ~Base()
    {
        std::cout << "Base析构函数" << std::endl;
    }
};

class Son : public Base
{
private:
    int num;
public:
    Son(int num1, int num2) : num(num2), Base(num1)
    {
        std::cout << "Son有参构造 int int" << std::endl;
    }
    ~Son()
    {
        std::cout << "Son析构函数" << std::endl;
    }
    void showNum(void)
    {
        // 1. 当父类和子类成员变量同名时，在子类就近原则，选择本作用域的子类成员
        //    1.1父类的num是private，在子类中不可见
        // 2. 如果子类中必须使用父类中的同名成员变量，则必须使用父类的作用域
        std::cout << "父类中的num = " << Base::num << std::endl;
        std::cout << "子类中的num = " << num << std::endl;
    }
};

void test01()
{
    Son son(10, 20);
    son.showNum();
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}