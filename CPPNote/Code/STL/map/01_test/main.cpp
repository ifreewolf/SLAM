#include <iostream>
#include <map>
#include <string>
#include <algorithm>
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


}

int main(int argc, char **argv)
{
    test01();

    return 0;
}