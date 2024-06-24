#include <iostream>

class Animal
{
public:
    virtual void sleep(void)
    {
        std::cout << "animal 动态在睡觉" << std::endl;
    }
    virtual ~Animal()
    {
        std::cout << "Animal的析构" << std::endl;
    }
};

class Cat : public Animal
{
public:
    virtual void sleep(void)
    {
        std::cout << "猫在睡觉" << std::endl;
    }
    ~Cat()
    {
        std::cout << "Cat的析构" << std::endl;
    }
};

void test01()
{
    Animal* p = new Cat;
    p->sleep();
    
    // 出现的问题：
    delete p;   // 只调用了Animal的析构函数

    Cat* cat = new Cat;
    cat->sleep();
    delete cat;
}


int main(int argc, char **argv)
{
    test01();   
    return 0;
}