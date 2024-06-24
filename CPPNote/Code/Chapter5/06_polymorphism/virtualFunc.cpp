#include <iostream>

class Animal
{
public:
    // 虚函数，用virtual修饰的成员函数只要涉及到继承，子类中同名函数都是虚函数
    // 虚函数，本质是一个函数指针变量
    // 如果Animal不涉及到继承，函数指针变量就指向自身sleep
    virtual void sleep(void)
    {
        std::cout << "animal 动态在睡觉" << std::endl;
    }
};

class Cat : public Animal
{
public:
    // 子类的同名函数不管是否使用virtual关键字修饰，都默认为virtual函数
    virtual void sleep(void)
    {
        std::cout << "猫在睡觉" << std::endl;
    }
};

void test01()
{
    // 需求：用基类(指针或引用)保存子类对象，同时还需要操作子类自身成员
    Animal *p = new Cat;
    p->sleep(); // 猫在睡觉
}


int main(int argc, char **argv)
{
    test01();   
    return 0;
}