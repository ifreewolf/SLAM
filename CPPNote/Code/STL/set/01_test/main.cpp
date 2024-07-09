#include <iostream>
#include <set>
#include <algorithm>

using namespace std;

/*
1. set构造函数
set<T> st; // set容器默认构造函数
multiset<T> mst; // multiset容器默认构造函数
set(const set &st); // 拷贝构造函数

2. set赋值操作
set& operator=(const set &st); // 重载等号操作符
swap(st); // 交换两个集合容器

3. set大小操作
size(); // 返回容器中元素的数目
empty(); // 判断容器是否为空

4. set插入和删除操作
insert(elem); // 在容器中插入元素
clear(); // 清除所有元素
erase(pos); // 删除pos迭代器所指的元素，返回下一个元素的迭代器
erase(beg, end); // 删除区间[beg, end)的所有元素，返回下一个元素的迭代器
erase(elem); // 删除容器中值为elem的元素
*/
void test01()
{
    set<int> s;
    s.insert(30);
    s.insert(10);
    s.insert(20);
    s.insert(50);
    s.insert(40);

    for_each(s.begin(), s.end(), [](int val){ cout << val << " "; }); // 10 20 30 40 50 
    cout << endl;

    set<int>::iterator it = s.begin();
    // set容器提供的是只读迭代器const_iterator
    // 迭代器不允许修改值
    // *it = 100; // error: assignment of read-only location ‘it.std::_Rb_tree_const_iterator<int>::operator*()'

    cout << "size = " << s.size() << endl; // size = 5

    s.erase(s.begin()); // 删除起始位置
    for_each(s.begin(), s.end(), [](int val){ cout << val << " "; }); // 20 30 40 50
    cout << endl;

    s.erase(40); // 根据元素删除
    for_each(s.begin(), s.end(), [](int val){ cout << val << " "; }); // 20 30 50
    cout << endl;
}

/*
set查找操作
find(key); // 查找key是否存在，若存在，返回该键的元素的迭代器；若不存在，返回set.end();
count(key); // 查找键key的元素个数
lower_bound(keyElem); // 返回第一个key>=keyElem元素的迭代器
upper_bound(keyElem); // 返回第一个key>keyElem元素的迭代器。
equal_range(keyElem); // 返回容器中key与keyElem相等的上下限的两个迭代器。
*/
void test02()
{
    set<int> s;
    s.insert(30);
    s.insert(10);
    s.insert(20);
    s.insert(50);
    s.insert(40);

    // find(key); // 查找key是否存在，若存在，返回该键的元素的迭代器；若不存在，返回set.end();
    set<int>::const_iterator it = s.find(20);
    if (it == s.end()) {
        cout << "没有找到" << endl;
    } else {
        cout << "找到" << *it << endl;
    }

    // count(key); // 查找键key的元素个数
    // set容器的键值是不重复的，那么count(key)只能是0或1。
    int cnt = s.count(30);
    cout << "cnt = " << cnt << endl; // cnt = 1
    cout << s.count(200) << endl; // 0
}

void test03()
{
    set<int> s;
    s.insert(30);
    s.insert(10);
    s.insert(20);
    s.insert(50);
    s.insert(40);

    for_each(s.begin(), s.end(), [](int val) { cout << val << " "; }); // 10 20 30 40 50
    cout << endl;
    // lower_bound(keyElem); // 返回第一个key>=keyElem元素的迭代器，下限
    set<int>::const_iterator it = s.lower_bound(30);
    if (it != s.end()) {
        cout << "下限查找:" << *it << endl; // 下限查找:30，下限查找:30
    } else {
        cout << "下限查找:不存在" << endl;
    }
    // upper_bound(keyElem); // 返回第一个key>keyElem元素的迭代器，上限
    it = s.upper_bound(30);
    if (it != s.end()) {
        cout << "上限查找:" << *it << endl; // 下限查找:30，上限查找:40
    } else {
        cout << "上限查找:不存在" << endl;
    }
}

void test04()
{
    set<int> s;
    s.insert(30);
    s.insert(10);
    s.insert(20);
    s.insert(50);
    s.insert(40);

    for_each(s.begin(), s.end(), [](int val) { cout << val << " "; }); // 10 20 30 40 50
    cout << endl;

    // equal_range(keyElem); // 返回容器中key与keyElem相等的上下限的两个迭代器。
    // equal_range 返回的是对组
    // first对组中第一个值，second对组中第二个值
    pair<set<int>::const_iterator, set<int>::const_iterator> pit = s.equal_range(30);
    if (pit.first != s.end()) {
        cout << "下限查找:" << *(pit.first) << endl; // 下限查找:30
    } else {
        cout << "下限查找:不存在" << endl;
    }

    if (pit.second != s.end()) {
        cout << "上限查找:" << *(pit.second) << endl; // 上限查找:40
    } else {
        cout << "上限查找:不存在" << endl;
    }
}

/*
pair对组
*/
void test05()
{
    // 9527--星爷 10086--移动 10010--联通
    // 定义对组方式1：
    pair<int, string> pair1(9527, "星爷");
    cout << "编号：" << pair1.first << ", 人物：" << pair1.second << endl; // 编号：9527, 人物：星爷

    // 定义对组方式2：(推荐)
    pair<int, string> pair2 = make_pair(10086, "移动");
    cout << "编号：" << pair2.first << ", 人物：" << pair2.second << endl; // 编号：10086, 人物：移动

    // pair=赋值
    pair<int ,string> pair3 = pair2;
    cout << "编号：" << pair2.first << ", 人物：" << pair2.second << endl; // 编号：10086, 人物：移动
}

class MyGreater
{
public:
    bool operator()(int val1, int val2)
    {
        return val1 > val2;
    }
};

void test06()
{
    // 默认从小到大排序
    // 改成从大到小排序,一旦插入过后无法修改排序规则
    // set<int, 排序规则> s;
    // set<int, std::greater<int>> s; // 方法一
    set<int, MyGreater> s; // 方法二,自定义排序规则
    s.insert(30);
    s.insert(10);
    s.insert(20);
    s.insert(50);
    s.insert(40);
    for_each(s.begin(), s.end(), [](int val) { cout << val << " "; }); // 50 40 30 20 10
    cout << endl;

}

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

    // 方法一:重载<运算符 从小到大
    bool operator<(const Person &ob) const
    {
        return this->age < ob.age;
    }
};
void test07()
{
    set<Person> s;
    // set存放自定义数据,需提供排序规则,否则无法在内存中进行顺序存放
    s.insert(Person("德玛西亚", 18)); // error: no match for ‘operator<’ (operand types are ‘const Person’ and ‘const Person’)
    s.insert(Person("冯楠", 21)); 
    s.insert(Person("小炮", 20)); 
    s.insert(Person("小法", 19)); 

    for_each(s.begin(), s.end(), [](const Person &ob){ cout << "姓名:" << ob.name << ", 年龄:" << ob.age << endl;});
}

class MyGreaterPerson
{
public:
    bool operator()(const Person &ob1, const Person &ob2)
    {
        return ob1.age > ob2.age;
    }
};

void test08()
{
    set<Person, MyGreaterPerson> s;
    s.insert(Person("德玛西亚", 18));
    s.insert(Person("冯楠", 21)); 
    s.insert(Person("小炮", 20)); 
    s.insert(Person("小法", 19)); 
    for_each(s.begin(), s.end(), [](const Person &ob){ cout << "姓名:" << ob.name << ", 年龄:" << ob.age << endl;});
}

void test09()
{
    set<int> s;
    pair<set<int>::const_iterator, bool> it1;
    pair<set<int>::const_iterator, bool> it2;
    it1 = s.insert(10);
    it2 = s.insert(10);
    if (it1.second == true) {
        cout << "第一次插入成功" << endl; // 第一次插入成功
    } else {
        cout << "第一次插入失败" << endl;
    }
    
    if (it2.second == true) {
        cout << "第二次插入成功" << endl;
    } else {
        cout << "第二次插入失败" << endl; // 第二次插入失败
    }
}

void test10()
{
    multiset<int> mst;
    mst.insert(10);
    mst.insert(10);
    for_each(mst.begin(), mst.end(), [](int val){ cout << val << " ";}); // 10 10,两个相同元素插入成功
    cout << endl;
}

int main(int argc, char **argv)
{
    test10();

    return 0;
}