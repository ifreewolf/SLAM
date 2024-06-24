#include <iostream>

class Animal
{
public:
    // 纯虚函数：如果一个类中拥有纯虚函数，那么这个类就是抽象类
    // 抽象类不能实例化对象
    virtual void sleep() = 0;
    virtual ~Animal() { std::cout << "Animal dtor" << std::endl; }
};

class Cat : public Animal
{
public:
    virtual void sleep()
    {
        std::cout << "Cat在睡觉" << std::endl;
    }
    ~Cat()
    {
        std::cout << "Cat dtor" << std::endl;
    }
};

void test01()
{
    Animal* p = new Cat;
    p->sleep();
    delete p;

    // Animal抽象类不能实例化对象
    // Animal ob;  // error: cannot declare variable ‘ob’ to be of abstract type ‘Animal’
    Animal* animal; // 但这样是允许的，因为这只是声明了一个指针，而不是初始化对象
}

int main(int argc, char **argv)
{
    test01();
    return 0;
}