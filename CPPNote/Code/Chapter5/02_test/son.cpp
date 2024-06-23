#include <iostream>

class Base
{
public:
    int a;
protected:
    int b;
private:
    int c;
};

class Son : public Base
{
public:
    int d;
    int e;
};


void test()
{

    std::cout << sizeof(Son) << std::endl;  // 20 = 5 * 4字节
    std::cout << offsetof(Son, a) << std::endl;
    std::cout << offsetof(Son, d) << std::endl;
    std::cout << offsetof(Son, e) << std::endl;
}

int main(int argc, char **argv)
{
    test();

    return 0;
}