#include <iostream>
#include <map>
#include <string>
#include <algorithm>
#include <vector>
#include <numeric> // 极少数算法
using namespace std;

/*
1. map构造函数
map<T1, T2> mapTT; // map默认构造函数
map(const map &mp); // 拷贝构造函数

2. map赋值操作
map& operator=(const map &mp); // 重载等号操作符
swap(mp); // 交换两个集合容器

3. map大小操作
size(); // 返回容器中元素的数目
empty(); // 判断容器是否为空

4. map插入数据元素操作
insert(...); // 往容器插入元素，返回pair<iterator, bool>
map<int, string> mapStu;
// 第一种 通过pair的方式插入对象
mapStu.insert(pair<int, string>(3, "小张"));
// 第二种 通过pair的方式插入对象
mapStu.insert(make_pair(-1, "小张"));
// 第三种 通过value_type的方式插入对象
mapStu.insert(map<int, string>::value_type(2, "小陈"));
// 第四种 通过数组的方式插入值
mapStu[3] = "小刘";
mapStu[5] = "小王";

5. map删除操作
clear(); // 删除所有元素
erase(pos); // 删除pos迭代器所指的元素，返回下一个元素的迭代器
erase(beg, end); // 删除区间[beg, end)的所有元素，返回下一个元素的迭代器
erase(keyElem); // 删除容器中key为keyElem的对组

6. map查找操作
find(key); // 查找key是否存在，若存在，返回该键的元素的迭代器；若不存在，返回map.end()
count(keyElem); // 返回容器中key为keyElem的对组个数。对map来说，要么是0，要么是1
lower_bound(keyElem); // 返回第一个key>=keyElem元素的迭代器
upper_bound(keyElem); // 返回第一个key>keyElem元素的迭代器
equal_bound(keyElem); // 返回容器中key与keyElem相等的上下限的两个迭代器
*/
void test01()
{
    // 9527--"星爷" 10086--"移动" 10010--"联通"
    // int为键值的类型，string为实值的类型
    map<int, string> mp;
    // 第一种：
    mp.insert(pair<int, string>(9527, "星爷"));
    // 第二种：(推荐)
    mp.insert(make_pair(10086, "移动"));
    // 第三种：使用value_type
    mp.insert(map<int, string>::value_type(10010, "联通"));
    // 第四种：读map容器数据的时候，推荐
    mp[10000] = "电信";
    // mp[10] 寻找key为10的实值
    // 如果map容器中没有key为10，使用mp[10]会创建一个key值为10，实值为空的对组。
    // 如果容器中有key为10，那么mp[10]代表key=10的实值。
    // 建议：10是存在的
    // cout << mp[10] << endl;

    // 遍历
    for_each(mp.begin(), mp.end(), [](pair<int, string> p){cout << p.first << ":" << p.second << endl;});

    // 只想查看key==9527的实值，保证key是存在的
    cout << mp[9527] << endl;
    cout << mp[10010] << endl;

    // 如果不能确定key值是否存在
    map<int, string>::const_iterator it = mp.find(10086);
    if (it == mp.end()) {
        cout << "未找到相关节点" << endl;
    } else {
        cout << "找到相关节点: " << it->second <<  endl; // 找到相关节点: 移动
    }
}

void test02()
{
    // 设置种子
    srand(time(NULL));
    // 战队容器（战队编号，战队名称）
    map<int, string> m;
    m.insert(make_pair(1, "RNG"));
    m.insert(make_pair(2, "IG"));
    m.insert(make_pair(3, "WE"));
    m.insert(make_pair(4, "EDG"));

    // 使用vector存放战队编号
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);

    // 随机抽签（洗牌）
    random_shuffle(v.begin(), v.end());

    // 随机出场
    for_each(v.begin(), v.end(), [&m] (int key){
        cout << m[key] << "战队出场了" << endl;
    });
}

void test03()
{
    // 战队容器（战队编号，战队名称）
    map<int, string> m;
    m.insert(make_pair(1, "RNG"));
    m.insert(make_pair(1, "IG"));

    cout << m[1] << endl;
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
};
void createVectorPerson(vector<Person> &v)
{
    v.push_back(Person("员工A", 21));
    v.push_back(Person("员工B", 23));
    v.push_back(Person("员工C", 21));
    v.push_back(Person("员工D", 22));
    v.push_back(Person("员工E", 21));
}
void PersonByGroup(vector<Person> &v, multimap<int, Person> &m)
{
    // 逐个将员工分配到各个部门内
    for(vector<Person>::iterator it = v.begin(); it != v.end(); it++) {
        cout << "请输入<" << it->name << ">将要加入的部门:1 2 3" << endl;
        int dep = 0;
        cin >> dep;
        if (dep >= 1 && dep <= 3) {
            m.insert(make_pair(dep, *it));
        }
    }
}
void showPersonList(multimap<int, Person> &m, int dep)
{
    switch (dep)
    {
    case 1:
        cout << "研发部:" << endl;
        break;
    case 2:
        cout << "测试部:" << endl;
        break;
    case 3:
        cout << "人事部:" << endl;
        break;
    default:
        cout << "请输入正确的部门编号：1 2 3" << endl;
        break;
    }

    // 注意：m中存放的数据《部门号、员工信息》
    // 统计部门号的元素个数
    int n = m.count(dep);
    cout << "部门的人数个数： " << n << endl;
    // 由于multikey将key自动排序，重复相同key一定挨在一起
    multimap<int, Person>::const_iterator it = m.find(dep);
    if (it != m.end()) {
        for (int i = 0; i < n; i++, it++) {
            cout << "name = " << it->second.name << ", age = " << it->second.age << endl;
        }
    }
}
void test04()
{
    // 使用vector容器将员工信息存储起来
    vector<Person> v;
    // 创建5名员工
    createVectorPerson(v);

    // 将员工分配到不同的部门（员工<部门号，员工信息>）
    multimap<int, Person> m; // 存放员工<部门号，员工信息>
    PersonByGroup(v, m);

    // 按部门查看员工信息
    showPersonList(m, 1);
    showPersonList(m, 2);
    showPersonList(m, 3);
}

int main(int argc, char **argv)
{
    test04();

    return 0;
}