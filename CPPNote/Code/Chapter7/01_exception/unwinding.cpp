#include <iostream>
#include <string>

class Person
{
private:
    std::string name;
public:
    Person(std::string name)
    {
        this->name = name;
        std::cout << "Person " << name << " 构造函数" << std::endl;
    }
    ~Person()
    {
        std::cout << "Person " << this->name << " 析构函数" << std::endl;
    }
};


void test01()
{
    try {
        Person ob1("00_德玛");
        Person ob2("01_小炮");
        Person ob3("02_小法");
        Person ob4("03_提莫");

        throw 10;
    } catch (int e) {
        std::cout << "捕获到int异常 e = " << e << std::endl;
    }
}

int main(int argc, char **argv)
{
    test01();
    return 0;
}