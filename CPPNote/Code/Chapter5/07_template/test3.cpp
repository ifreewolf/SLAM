#include <iostream>
#include <string>

// 严格来说：类模板的类型不是Person而是Person<T1, T2>，传不同的类型代表不同的类
template<class T1, class T2>
class Person
{
private:
    T1 name;
    T2 age;
public:
    // 类内声明
    Person(T1 name, T2 age);
    ~Person();
    void showPerson();
};

// 类外定义
template<class T1, class T2>
// Person::Person(T1 name, T2 age)  // Person作用域是不对的，因为类型是Person<T1, T2>
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

template<class T1, class T2>
Person<T1, T2>::~Person()
{
    std::cout << "析构函数" << std::endl;
}

int main(int argc, char **argv)
{
    Person<std::string, int> ob1("德玛西亚", 18);
    ob1.showPerson();

    Person<int, int> ob2(100, 200);
    ob2.showPerson();

    return 0;
}