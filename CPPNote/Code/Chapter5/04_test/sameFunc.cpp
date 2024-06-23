#include <iostream>

class Base
{
public:
    void func(void)
    {
        std::cout << "父类中的void func" << std::endl;
    }
    void func(int a)
    {
        std::cout << "父类中的int func a = " << a << std::endl;
    }
};

class Son : public Base
{
public:
    void func(void)
    {
        std::cout << "子类中的void func" << std::endl;
    }
};

void test01()
{
    Son son;
    son.func();     // 访问子类的func void
    // son.func(10);   // error: no matching function for call to ‘Son::func(int)’

    son.Base::func();
    son.Base::func(10);
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}