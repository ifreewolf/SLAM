#include <string>
#include "myarray.hpp"

class Person
{
    friend std::ostream& operator<<(std::ostream &out, const Person &ob);
private:
    std::string name;
    int age;
public:
    Person(){}
    Person(std::string name, int age) {
        this->name = name;
        this->age = age;
    }
};

std::ostream& operator<<(std::ostream &out, const Person &ob)
{
    out << "name = " << ob.name << ", age = " << ob.age << std::endl;
    return out;
}

int main(int argc, char **argv)
{
    MyArray<char> ob(10);
    ob.push_back('a');
    ob.push_back('b');
    ob.push_back('c');
    ob.push_back('d');
    ob.printArray();

    MyArray<int> ob2(10);
    ob2.push_back(10);
    ob2.push_back(20);
    ob2.push_back(30);
    ob2.push_back(40);
    ob2.push_back(50);
    ob2.printArray();

    // 存放自定义数据
    MyArray<Person> ob3(10);
    ob3.push_back(Person("德玛", 18));
    ob3.push_back(Person("西亚", 19));
    ob3.push_back(Person("小法", 20));
    ob3.push_back(Person("米兰", 21));
    ob3.push_back(Person("艾米", 22));
    ob3.printArray();

    return 0;
}