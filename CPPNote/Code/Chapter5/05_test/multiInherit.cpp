#include <iostream>

class Base1
{
public:
    int a;
};
class Base2
{
public:
    int a;
    int b;
};

class Son : public Base1, public Base2
{

};

void test01()
{
    Son son;
    // std::cout << son.a << std::endl;   // error: request for member ‘a’ is ambiguous
    std::cout << son.b << std::endl;
    std::cout << son.Base1::a << ", " << son.Base2::a << std::endl;
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}