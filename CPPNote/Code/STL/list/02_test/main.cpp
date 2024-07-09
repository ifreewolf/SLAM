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
    // bool operator<(const Person &ob)
    // {
    //     return this->age < ob.age;
    // }
};

void printListPerson(list<Person> &l)
{
    for (list<Person>::iterator it = l.begin(); it != l.end(); it++) {
        cout << it->name << ", " << it->age << endl;
    }
}

void printVectorPerson(vector<Person> &v)
{
    cout << "=====================" << endl;
    for (vector<Person>::iterator it = v.begin(); it != v.end(); it++) {
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

    // 对于自定义数据，可以重载<运算符
    // l.sort(); // error: no match for ‘operator<’ (operand types are ‘Person’ and ‘Person’)
    printListPerson(l);
}

bool myComparePerson(const Person &ob1, const Person &ob2)
{
    return ob1.age < ob2.age;
}

void test03()
{
    list<Person> l;
    l.push_back(Person("德玛西亚", 18));
    l.push_back(Person("提莫", 28));
    l.push_back(Person("狗头", 38));
    l.push_back(Person("牛头", 19));
    printListPerson(l);
    cout << "==================" << endl;

    l.sort(myComparePerson);
    printListPerson(l);
}

void test04()
{
    vector<Person> v;
    v.push_back(Person("德玛西亚", 18));
    v.push_back(Person("提莫", 28));
    v.push_back(Person("狗头", 38));
    v.push_back(Person("牛头", 19));
    printVectorPerson(v);

    // 默认比较方式从小到大，对于自定义数据没有排序规则
    // vector存放自定义数据，可指定排序规则
    sort(v.begin(), v.end(), myComparePerson);
    printVectorPerson(v);
}

// 仿函数指定排序规则
class MyComparePerson
{
public:
    bool operator()(const Person &ob1, const Person &ob2)
    {
        return ob1.age < ob2.age;
    }
};

void test05()
{
    vector<Person> v;
    v.push_back(Person("德玛西亚", 18));
    v.push_back(Person("提莫", 28));
    v.push_back(Person("狗头", 38));
    v.push_back(Person("牛头", 19));
    printVectorPerson(v);

    // 默认比较方式从小到大，对于自定义数据没有排序规则
    // vector存放自定义数据，可指定排序规则
    sort(v.begin(), v.end(), MyComparePerson());
    printVectorPerson(v);
}

class MyAdd
{
public:
    // 本质是成员函数，函数名叫operator()
    int operator()(int a, int b)
    {
        cout << "调用了operator() int int" << endl;
        return a + b;
    }
    int operator()(int a, int b, int c)
    {
        cout << "调用了operator() int int int" << endl;
        return a + b + c;
    }
};
void test06()
{
    MyAdd ob1;
    int ret = ob1.operator()(10, 20);
    cout << ret << endl; // 30
    // 严格意义：ob1是对象和()结合，调用operator()成员函数
    // ob1(100,200) ob1本质是对象不是函数名，只是形式像函数调用
    // 叫作仿函数
    cout << ob1(20, 50) << endl; // 70
    cout << MyAdd()(40, 90) << endl; // 130

    cout << ob1(10, 20, 30) << endl; // 60

    cout << MyAdd()(1000, 2000) << endl; // 3000
}

void myPrintVectorInt(vector<int> &v)
{
    for (vector<int>::iterator it = v.begin(); it != v.end(); it++) {
        cout << *it << " ";
    }
    cout << endl;
}

// 指定打印方式
void myPrint01(int val)
{
    cout << val << " ";
}

void test07()
{
    vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    v.push_back(40);

    // 方法一：访问v容器，普通函数
    myPrintVectorInt(v);

    // 方法二：访问v容器，系统算法for_each
    for_each(v.begin(), v.end(), myPrint01);
    cout << endl;

    // 方法三：访问v容器 lambda表达式
    // []标识函数名 ()参数列表 {}函数体
    for_each(v.begin(), v.end(), [](int val){ cout << val << " "; });
    cout << endl;
}

int main(int argc, char **argv)
{
    test07();

    return 0;
}