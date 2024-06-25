#include <iostream>

void mySwap(int a, int b)
{
    std::cout << "普通函数" << std::endl;
}

template<typename T>
void mySwap(T a, T b)
{
    std::cout << "函数模板" << std::endl;
}

void test01()
{
    int a = 100;
    char b = 'a';
    // 函数模板的参数类型，不能自动类型转换
    // 普通函数的参数类型，可以自动类型转换
    mySwap(a, b);   // 普通函数，这两个函数都没有引用

    // 编译器自动转换指定T
    // mySwap<>(a, b); // error: no matching function for call to ‘mySwap(int&, char&)’
    mySwap<int>(a, b);  // 函数模板，指定T之后，相当于普通函数；因为自动转换编译器不知道该转换成int还是char
}

int main(int argc, char **argv)
{
    test01();
    return 0;
}