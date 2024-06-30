#include <iostream>

class Person
{
private:
    int age;
public:
    Person(int age)
    {
        if (age <0 || age > 150) {
            throw std::out_of_range("age无效");
        }
        this->age = age;
    }
};

void test01()
{
    try {
        Person ob(200);
    } catch (std::exception &e) {
        std::cout << "捕获到异常" << e.what() << std::endl; // 捕获到异常age无效
    }
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}