#include <iostream>

// 自定义异常类
class MyOutOfRange : public std::exception
{
public:
    MyOutOfRange(const std::string errorInfo)
    {
        this->m_Error = errorInfo;
    }
    MyOutOfRange(const char* errorInfo)
    {
        this->m_Error = std::string(errorInfo);
    }
    virtual ~MyOutOfRange()
    {

    }
    virtual const char* what() const noexcept
    {
        return this->m_Error.c_str();
    }
    std::string m_Error;
};

class Person
{
public:
    Person(int age)
    {
        if (age <= 0 ||age > 150) {
            // 抛出异常越界
            // std::cout << "越界" << std::endl;
            // throw std::out_of_range("年龄必须在0~150之间");

            // throw length_error("长度异常");
            throw MyOutOfRange("我的异常 年龄必须在0~150之间");
        } else {
            this->m_Age = age;
        }
    }
    int m_Age;
};

int main(int argc, char **argv)
{
    try {
        Person p(151);
    }
    catch (std::out_of_range &e) {
        std::cout << e.what() << std::endl;
    }
    catch (std::length_error &e) {
        std::cout << e.what() << std::endl;
    }
    catch (MyOutOfRange e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}