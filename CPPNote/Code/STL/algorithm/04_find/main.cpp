#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

class Person
{
public:
    string name;
    int age;
    Person(string name, int age)
    {
        this->name = name;
        this->age = age;
    }
    bool operator==(const Person &ob)
    {
        return this->name == ob.name && this->age == ob.age;
    }
};

void test01()
{
    vector<int> v1;
    v1.push_back(10);
    v1.push_back(20);
    v1.push_back(30);
    v1.push_back(40);

    vector<int>::iterator it;
    it = find(v1.begin(), v1.end(), 20);
    if (it != v1.end()) {
        cout << "找到了" << endl;
    } else {
        cout << "未找到" << endl;
    }

    vector<Person> v2;
    v2.push_back(Person("德玛西亚", 18));
    v2.push_back(Person("小法", 19));
    v2.push_back(Person("小炮", 20));
    v2.push_back(Person("牛头", 21));
    vector<Person>::iterator pIt;
    Person tmp("小炮", 20);
    // find对于自定义数据，需要重载==运算符
    pIt = find(v2.begin(), v2.end(), tmp);
    if (pIt != v2.end()) {
        cout << "找到了, name = " << pIt->name << ", age = " << pIt->age << endl;
    } else {
        cout << "未找到" << endl;
    }

}

bool myGreaterThan20(int val)
{
    return val > 20;
}
class MyGreaterThan20
{
public:
    bool operator()(int val)
    {
        return val > 20;
    }
};
void test02()
{
    vector<int> v1;
    v1.push_back(10);
    v1.push_back(20);
    v1.push_back(30);
    v1.push_back(40);

    // 寻找第一个大于20的数
    vector<int>::iterator it;
    // it = find_if(v1.begin(), v1.end(), myGreaterThan20);
    it = find_if(v1.begin(), v1.end(), MyGreaterThan20());
    if (it != v1.end()) {
        cout << "找到了" << endl;
    } else {
        cout << "未找到" << endl;
    }
}

/*
adjacent_find
*/
void test03()
{
    vector<int> v1;
    v1.push_back(10);
    v1.push_back(20);
    v1.push_back(30);
    v1.push_back(30);
    v1.push_back(40);
    v1.push_back(50);
    v1.push_back(50);

    vector<int>::iterator it;
    // 对于普通数据，不需要回调函数
    it = adjacent_find(v1.begin(), v1.end());
    if (it != v1.end()) {
        cout << "寻找到重复的数据：" << *it << endl; // 30
    } else {
        cout << "不存在重复的数据" << endl;
    }

    // 对于自定义数据，需要提供operator==运算符
    vector<Person> v2;
    v2.push_back(Person("德玛西亚", 18));
    v2.push_back(Person("小法", 19));
    v2.push_back(Person("小炮", 20));
    v2.push_back(Person("小炮", 20));
    v2.push_back(Person("牛头", 21));
    vector<Person>::iterator pit;
    pit = adjacent_find(v2.begin(), v2.end());
    if (pit != v2.end()) {
        cout << "寻找到重复的数据： name = " << pit->name << ", age = " << pit->age << endl; // 寻找到重复的数据： name = 小炮, age = 20
    } else {
        cout << "不存在重复的数据" << endl;
    }
}

/*
binary_search
*/
void test04()
{
    vector<int> v1;
    v1.push_back(10);
    v1.push_back(20);
    v1.push_back(30);
    v1.push_back(40);
    v1.push_back(50);

    bool ret = binary_search(v1.begin(), v1.end(), 30);
    if (ret) {
        cout << "找到了30" << endl; // 找到了30
    } else {
        cout << "未找到" << endl;
    }
}

/*
count
*/
void test05()
{
    vector<int> v1;
    v1.push_back(10);
    v1.push_back(20);
    v1.push_back(10);
    v1.push_back(40);
    v1.push_back(50);
    v1.push_back(10);

    int cnt = count(v1.begin(), v1.end(), 10);
    cout << "10的个数：" << cnt << endl; // 3
}

/*
count_if
*/
void test06()
{
    vector<int> v1;
    v1.push_back(10);
    v1.push_back(20);
    v1.push_back(30);
    v1.push_back(40);
    v1.push_back(50);

    int cnt = count_if(v1.begin(), v1.end(), bind2nd(greater_equal<int>(), 20));
    cout << "大于等于20的个数：" << cnt << endl; // 4
}

int main(int argc, char **argv)
{
    test06();
    return 0;
}