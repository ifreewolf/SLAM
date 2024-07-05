#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

void test01()
{
    vector<int> v;
    int capacity = v.capacity();
    int sz = v.size();
    cout << "容器内存：" << &v[0] << ", 扩充容器大小为：" << v.capacity() << endl;
    for (int i = 0; i < 100; i++) {
        v.push_back(i);
        if (v.capacity() != capacity) {
            cout << "更新了容器内存：" << &v[0] << ", 扩充容器大小为：" << v.capacity() << endl;
            capacity = v.capacity();
        }
    }

    cout << "v的容量capacity: " << v.capacity() << endl;    // v的容量capacity: 128
    cout << "v的大小size: " << v.size() << endl;    // v的大小size：100
}

void test02()
{
    vector<int> v;
    int *p = nullptr;
    for (int i = 0; i < 1000000000; i++) {
        if (p != &v[0]) {
            cout << "------------------" << i << "-----------------" << std::endl;
            cout << "capacity = " << v.capacity() << ", size = " << v.size() << endl;
            p = &v[0];
        }
        v.push_back(i);
    }
}

/*
vector的构造函数
vector<T> v; // 采用模板实现类对象，默认构造函数
vector(v.begin(), v.end()); // 将v[begin(), end()]区间中的元素拷贝给本身
vector(n, elem); // 构造函数将n个elem拷贝给本身
vector(const vector &vec); // 拷贝构造函数
*/
void test03()
{
    // vector(n, elem); // 构造函数将n个elem拷贝给本身
    vector<int> v1(10, 5);
    for_each(v1.begin(), v1.end(), [](int val){ cout << val << " ";});
    cout << endl;

    // vector(v.begin(), v.end()); // 将v[begin(), end()]区间中的元素拷贝给本身
    vector<int> v2(v1.begin(), v1.end() - 2);
    for_each(v2.begin(), v2.end(), [](int val){ cout << val << " ";});
    cout << endl;

    vector<int> v3(v1);
    for_each(v3.begin(), v3.end(), [](int val){ cout << val << " ";});
    cout << endl;

    // 使用int数组赋值
    int arr[] = {2, 3, 4, 1, 9};
    vector<int> v4(arr, arr + sizeof(arr) / sizeof(int));
    for_each(v4.begin(), v4.end(), [](int val){ cout << val << " ";});  // 2 3 4 1 9
    cout << endl;
}

void printVectorInt(vector<int> &v)
{
    for (vector<int>::iterator it = v.begin(); it != v.end(); it++) {
        cout << *it << " ";
    }
    cout << endl;
}

/*
vector常用赋值操作
assign(beg, end); // 将[beg,end)区间中的数据拷贝赋值给本身
assign(n, elem); // 将n个elem拷贝赋值给本身
vector& operator=(const vector &vec); // 重载等号操作符
swap(vec);  // 将vec与自身的元素互换
*/
void test04()
{
    vector<int> v1(5, 10);
    vector<int> v2;

    // vector& operator=(const vector &vec); // 重载等号操作符，是深拷贝。
    v2 = v1;
    printVectorInt(v2); // 10 10 10 10 10 
    v2[1] = 5;
    printVectorInt(v1); // 10 10 10 10 10 
    printVectorInt(v2); // 10 5 10 10 10 

    // assign(n, elem); // 将n个elem拷贝赋值给本身
    vector<int> v3;
    v3.assign(5, 100);
    printVectorInt(v3);

    // assign(beg, end); // 将[beg,end)区间中的数据拷贝赋值给本身
    vector<int> v4;
    v4.assign(v3.begin(), v3.end());
    printVectorInt(v4);

    // swap(vec);  // 将vec与自身的元素互换
    vector<int> v5(5, 20);
    vector<int> v6(10, 40);
    printVectorInt(v5);
    printVectorInt(v6);
    v5.swap(v6);
    printVectorInt(v5);
    printVectorInt(v6);
}

/*
vector大小操作
size(); // 返回容器中元素的个数
empty(); // 判断容器是否为空
resize(int num); // 重新指定容器的长度为num，若容器变长，则以默认值填充新位置。
resize(int num, elem); // 重新指定容器的长度为num，若容器变长，则以elem值填充新位置。
capacity(); // 容器的容量
reserve(int len); // 容器预留len个元素长度，预留位置不初始化，元素不可访问
*/
void test05()
{
    vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    v.push_back(40);

    if (v.empty()) {
        cout << "v容器为空" << endl;
    } else {
        cout << "容器非空" << endl;
        cout << "size = " << v.size() << endl;
        cout << "capacity = " << v.capacity() << endl;
    }

    printVectorInt(v); // 10 20 30 40

    v.resize(8);
    printVectorInt(v); // 10 20 30 40 0 0 0 0
    v.resize(3);
    printVectorInt(v); // 10 20 30

    v.resize(8, 2);
    printVectorInt(v); // 10 20 30 2 2 2 2 2

    v.reserve(100); // 预留空间，但是不初始化，且不可访问
    printVectorInt(v); // 10 20 30 2 2 2 2 2
    cout << "v的容量： " << v.capacity() << endl; // 100
    cout << "v的大小：" << v.size() << endl; // 8

    v.resize(5);
    printVectorInt(v);
    cout << "v的容量： " << v.capacity() << endl; // 100, resize不会改变容器的容量
    cout << "v的大小：" << v.size() << endl; // 5

    v.resize(400, 8);
    printVectorInt(v);
    cout << "v的容量： " << v.capacity() << endl; // 400
    cout << "v的大小：" << v.size() << endl; // 400
}

void test06()
{
    vector<int> v;
    for (int i = 0; i < 1000; i++) {
        v.push_back(i);
    }
    cout << "size: " << v.size() << endl; // 1000
    cout << "capacity: " << v.capacity() << endl; // 1024

    // 使用resize将空间置为10个元素（可以吗？）
    v.resize(10); // 不能修改容量，只能修改size
    cout << "size: " << v.size() << endl; // 10
    cout << "capacity: " << v.capacity() << endl; // 1024

    // 使用swap收缩容器的容量
    cout << vector<int>(v).size() << endl;  // 10
    cout << vector<int>(v).capacity() << endl;  // 10
    vector<int>(v).swap(v);
    cout << "size: " << v.size() << endl; // 10
    cout << "capacity: " << v.capacity() << endl; // 10
}

// reserve(int len); // 容器预留len个元素长度，预留位置不初始化，元素不可访问
void test07()
{
    vector<int> v;

    // 一次性给够空间，叫空间预留
    v.reserve(1000); // 预留空间，1000个元素
    int *p = NULL;
    int count = 0;
    for (int i = 0; i < 1000; i++) {
        v.push_back(i);
        if (p != &v[0]) {
            count++;
            p = &v[0];
        }
    }
    cout << "重新另寻空间次数：" << count << endl; // 11，reserve(1000)之后，只另寻了1次
}

/*
vector数据存取操作
at(int idx); // 返回索引idx所指的数据，如果idx越界，抛出out_of_range异常。
operator[]; // 返回索引idx所指的数据，越界时，运行直接报错
front(); // 返回容器中的第一个数据元素
back(); // 返回容器中的最后一个数据元素
*/
void test08()
{
    vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    v.push_back(40);

    printVectorInt(v); // 10
    cout << v[2] << endl;    // 30
    cout << v.at(2) << endl; // 30

    // []越界不抛出异常
    // at越界抛出异常
    cout << v[100] << endl; // 0
    cout << v.at(100) << endl; // throwing an instance of 'std::out_of_range'

    cout << "front = " << v.front() << endl; // 10
    cout << "end = " << v.back() << endl; // 40
}

/*
vector的插入和删除
insert(const_iterator pos, int count, ele); // 迭代器指向位置pos插入count个ele元素
push_back(ele); // 尾部插入元素ele
pop_back(); // 删除最后一个元素
erase(const_iterator start, const_iterator end); // 删除迭代器从start到end之间的元素
erase(const_iterator pos); // 删除迭代器指向的元素
clear(); // 删除容器中所有元素
*/
void test09()
{
    vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    v.push_back(40);
    printVectorInt(v); // 10 20 30 40

    // insert(const_iterator pos, int count, ele); // 迭代器指向位置pos插入count个ele元素
    v.insert(v.begin() + 2, 3, 100);
    cout << "size = " << v.size() << endl; // 7
    cout << "capacity = " << v.capacity() << endl; // 8，按照动态扩充的逻辑在扩充容量
    printVectorInt(v); // 10 20 100 100 100 30 40

    // 尾部删除
    v.pop_back(); // 删除了40    
    cout << "size = " << v.size() << endl; // 6
    cout << "capacity = " << v.capacity() << endl; // 8
    printVectorInt(v); // 10 20 100 100 100 30

    // erase(const_iterator start, const_iterator end); // 删除迭代器从start到end之间的元素
    // [begin, end)
    v.erase(v.begin() + 2, v.end());
    cout << "size = " << v.size() << endl; // 2
    cout << "capacity = " << v.capacity() << endl; // 8
    printVectorInt(v); // 10 20

    // erase(const_iterator pos); // 删除迭代器指向的元素
    v.erase(v.begin() + 1);
    cout << "size = " << v.size() << endl; // 1
    cout << "capacity = " << v.capacity() << endl; // 8
    printVectorInt(v); // 10

    // clear(); // 删除容器中所有元素
    v.clear();
    cout << "size = " << v.size() << endl; // 0
    cout << "capacity = " << v.capacity() << endl; // 8
}

int main(int argc, char **argv)
{
    test09();
    return 0;
}