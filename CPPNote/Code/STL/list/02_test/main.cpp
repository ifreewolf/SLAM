#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <list>

using namespace std;

class Person
{
public:
    string name;
    int age;
public:
    Person(string name, int age)
    {
        this->name = name;
        this->age = age;
    }
    bool operator==(const Person &ob)
    {
        if (this->age == ob.age && this->name == ob.name) {
            return true;
        }
        return false;
    }
};

void printListPerson(list<Person> &l)
{
    for (list<Person>::iterator it = l.begin(); it != l.end(); it++) {
        cout << it->name << ", " << it->age << endl;
    }
}

void test01()
{
    // 存放自定义的数据
    list<Person> l;
    l.push_back(Person("德玛西亚", 18));
    l.push_back(Person("提莫", 28));
    l.push_back(Person("狗头", 38));
    l.push_back(Person("牛头", 19));
    printListPerson(l);

    // 删除狗头
    Person tmp("狗头", 38);
    l.remove(tmp); // error: no match for ‘operator==’ (operand types are ‘Person’ and ‘const value_type’ {aka ‘const Person’})
    printListPerson(l);
}

void test02()
{
    list<Person> l;
    l.push_back(Person("德玛西亚", 18));
    l.push_back(Person("提莫", 28));
    l.push_back(Person("狗头", 38));
    l.push_back(Person("牛头", 19));
    printListPerson(l);

    l.sort(); // error: no match for ‘operator<’ (operand types are ‘Person’ and ‘Person’)
}

int main(int argc, char **argv)
{
    test02();

    return 0;
}