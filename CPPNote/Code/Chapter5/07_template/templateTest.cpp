#include <iostream>

class Person
{
    friend std::ostream& operator<<(std::ostream &out, Person &ob);
    // template<> friend Person& myMax<Person>(Person &ob1, Person &ob2);
private:
    int a;
    int b;
public:
    Person(int a, int b) : a(a), b(b)
    {
        std::cout << "构造函数" << std::endl;
    }
    ~Person()
    {
        std::cout << "析构函数" << std::endl;
    }
    // 方法二：重载>运算符
    bool operator>(Person &ob)
    {
        return this->a > ob.a;
    }
};

std::ostream& operator<<(std::ostream &out, Person &ob)
{
    out << "a = " << ob.a << ", b = " << ob.b << std::endl;
    return out;
}

template<typename T>
T& myMax(T &a, T &b)
{
    return a > b ? a : b;   // error: no match for ‘operator>’ (operand types are ‘Person’ and ‘Person’)
}

// 方法一：提供函数模板具体化
// C++标准不直接支持模板函数作为友元（特别是模板特化），所以无法解决要在类外访问私有成员变量
template<> Person& myMax<Person>(Person &ob1, Person &ob2)
{
    return ob1.a > ob2.a ? ob1 : ob2;
}

// 方法二：重载>运算符

void test01()
{
    int data1 = 10, data2 = 20;
    std::cout << myMax(data1, data2) << std::endl;   // 20

    Person ob1(10, 20);
    Person ob2(100, 200);
    std::cout << myMax(ob1, ob2) << std::endl;
}

int main(int argc, char **argv)
{
    test01();
    return 0;
}