#include <iostream>

using namespace std;

// 设置一个父类
class Base
{
public:
    int a;
private:
    int b;
protected:
    int c;
};

// 设置一个子类
class Son : public Base
{
public:
    // 子类的内部
    void test()
    {
        a = 1000;
        // b = 2000; // 不可访问
        c = 3000;
    }
};

class GrandSon : public Son
{
public:
    void showGrandSon()
    {
        a = 10000;
        // b = 20000; // 不可访问
        c = 30000;
    }
};

class Son1 : protected Base
{
public:
    // 子类的内部
    void showSon()
    {
        a = 1000;
        // b = 2000; // 不可访问
        c = 3000;
    }
};

class GrandSon1 : public Son1
{
public:
    void showGrandSon()
    {
        a = 10000;
        // b = 20000; // 不可访问
        c = 30000;
    }
};

class Son2 : private Base
{
public:
    // 子类的内部
    void showSon()
    {
        a = 1000;
        // b = 2000; // 不可访问
        c = 3000;
        std::cout << "a = " << a << ", c = " << c << std::endl;
    }
};

class GrandSon2 : public Son2
{
public:
    void showGrandSon()
    {
        // a = 10000; // 不可访问
        // b = 20000; // 不可访问
        // c = 30000; // 不可访问
    }
};

void test01()
{
    // 子类的外部
    Son son;
    std::cout << "son a = " << son.a << std::endl;
    // std::cout << "son b = " << son.b << std::endl; // 不可访问
    // std::cout << "son c = " << son.c << std::endl; // 不可访问

    GrandSon grandSon;
    std::cout << "grandSon a = " << grandSon.a << std::endl;
    // std::cout << "grandSon b = " << grandSon.b << std::endl; // 不可访问
    // std::cout << "grandSon c = " << grandSon.c << std::endl; // 不可访问



};

// public继承分析：
// 1. 父类的public成员，在子类内、子类外以及孙子类内，外都可访问，说明父类的public成员在子类中依然是public权限；
// 2. 父类的protected成员，在子类内以及孙子类的内部都可以访问，但在类外均不可访问，说明父类的protected成员在子类中是protected的
// 3. 父类的private成员，在子类内、外，孙子类内、外均不可访问，说明父类private成员在子类中不可见，但依然存在。

// public继承总结：
// 1. 父类public成员，在子类中是public权限；
// 2. 父类protected成员，在子类中是protected权限；
// 3. 父类private成员，在子类中是不可见的。

void test02()
{
    Son1 son1;
    son1.showSon();
    // std::cout << son1.a << std::endl;   // 无法访问
}

// protected继承分析：
// 1. 父类的public成员在子类内部可以访问，在子类外部无法访问；在孙子类内部可以访问，说明父类的public成员在子类中是protected权限；
// 2. 父类的protected成员在子类内部可以访问，子类外部无法访问；在孙子类内部可以访问，外部无法访问，说明父类的protected成员在子类中依然是protected权限；
// 3. 父类的private成员在子类内部无法访问，说明父类的private成员在子类中是不可见的。

// protected继承总结：
// 1. 父类public成员，在子类是protected权限；
// 2. 父类protected成员，在子类中是protected权限；
// 3. 父类private成员，在子类中是不可见的。

void test03()
{
    Son2 son2;
    son2.showSon();

    GrandSon2 grandson2;
    grandson2.showGrandSon();
    // std::cout << son2.a << std::endl;    // 不可见
}

// private继承分析：
// 1. 父类的public成员在子类内部可以访问，在子类外部不可访问，且在孙子类中也无法访问，说明父类的public成员在子类中是private权限；
// 2. 父类的protected成员在子类内部可以访问，在孙子类中不可访问，在子类外部也不可访问，说明父类的protected成员在子类中是private权限;
// 3. 父类的private成员在子类无法访问，说明父类的private成员在子类中不可见。

// private继承总结：
// 1. 父类的public成员在子类是private权限；
// 2. 父类的protected成员在子类是private权限；
// 3. 父类的private成员在子类是不可见的。

int main(int argc, char **argv)
{
    test02();
    test03();

    return 0;
}