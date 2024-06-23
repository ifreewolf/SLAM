#include <iostream>

class Animal
{
public:
    Animal()
    {
        data = 100;
    }
    int data;
};

// 继承的动作：虚继承
// 父类：虚基类
class Horse : virtual public Animal
{
public:
    Horse()
    {
        data = 300;
    }
};

class Donkey : virtual public Animal
{
public:
    Donkey()
    {
        data = 400;
    }
};

class Mule : public Horse, public Donkey
{
};

void test01()
{
    Mule mule;
    mule.data = 200;
    std::cout << "data = " << mule.data << std::endl;

    // 通过Mule的vbptr寻找vbptr距离虚基类首地址的偏移量
    // &mule == vbptr
    // *(int *)&mule Donkey的虚基类表的起始位置
    std::cout << *(int*)((*(int*)&mule) + 1)<< std::endl;
    // int off_set = (int)*((int *)(*(int*)&mule) + 1);
    // // std::cout << off_set << std::endl;

    // // 通过sheep的vbptr和off_set定位虚基类的首地址
    // std::cout << ((Animal *)((char *)&mule +off_set))->data << std::endl;
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}