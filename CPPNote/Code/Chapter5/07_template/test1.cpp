#include <iostream>

void mySwap(int &a, int &b)
{
    std::cout << "普通函数" << std::endl;
    int tmp;
    tmp = a;
    a = b;
    b = tmp;
}

// class 和 typename一样的
template<typename T>
void mySwap(T &a, T &b)
{
    std::cout << "函数模板" << std::endl;
    T tmp = a;
    a = b;
    b = tmp;
}

void test01()
{
    int a = 1, b = 2;
    std::cout << "a = " << a << ", b = " << b << std::endl; // a = 1, b = 2
    mySwap<int>(a, b);
    std::cout << "a = " << a << ", b = " << b << std::endl; // a = 2, b = 1

    char c = 'a', d = 'b';
    std::cout << "c = " << c << ", d = " << d << std::endl; // c = a, d = b
    mySwap<char>(c, d);
    std::cout << "c = " << c << ", d = " << d << std::endl; // c = b, d = a
}

void test02()
{
    int data1 = 10, data2 = 20;
    // 函数模板和普通函数都识别时，优先选择普通函数
    mySwap(data1, data2);   // 普通函数

    // 函数模板和普通函数都识别，强行选择函数模板 加<>
    mySwap<>(data1, data2);

    // 函数模板参数的类型不能自动转换
    int a = 10;
    char b = 'b';
    // char无法强转为int&
    // mySwap(a, b);  // error: cannot bind non-const lvalue reference of type ‘int&’ to an rvalue of type ‘int’
}


int main(int argc, char **argv)
{
    test02();
    return 0;
}