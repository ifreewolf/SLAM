#include "person.h"

// 类外定义
template<class T1, class T2>
Person<T1, T2>::Person(T1 name, T2 age)
{
    std::cout << "有参构造" << std::endl;
    this->name = name;
    this->age = age;
}

template<class T1, class T2>
void Person<T1, T2>::showPerson()
{
    std::cout << "name = " << this->name << ", age = " << this->age << std::endl;
}