# 一、STL 概述

## 1.1 STL 基本概念

STL(Standard Template Library，标准模板库)

STL广义上三大组件：容器(container)、算法(algorithm)、迭代器(iterator)

<div align=center>
    <img src="./images/容器迭代器和算法关系.png" />
</div>

## 1.2 STL六大组件

容器(container) : 存放数据，vector、list、deque、set、map等
算法(algorithm) ： 操作数据，sort、find、copy、for_each
迭代器(iterator) : 容器和算法的桥梁，将operator*、operator->、operator++、operator--等指针相关操作予以重载的class template
仿函数(function) : 为算法提供更多的策略；行为类似函数，可作为算法的某种策略。从实现角度来看，仿函数是一种重载了operator()的class或者class template.
适配器(adoapter)：为算法提供更多的参数接口；用来修饰容器或者仿函数或迭代器接口的东西。
空间适配器 : 管理容器和算法的空间；负责空间的配置与管理。从实现角度看，配置器是一个实现了动态空间配置、空间管理、空间释放的class template.

### 1.2.1 算法的分类

质变算法：运算过程中会更改区间内的元素的内容；拷贝、替换、删除等等。
非质变算法：运算过程中不会更改容器内的数据；查找、计数、遍历、寻找极值等

### 1.2.2 迭代器

输入迭代器：提供对数据的只读访问；只读，支持++、\==、！=
输出迭代器：提供对数据的只写访问；只写，支持++
前向迭代器：提供读写操作，并能向前推进迭代器；读写，支持++、==、！=
双向迭代器：提供读写操作，并能向前和向后操作；读写，支持++、--
随机访问迭代器：提供读写操作，并能以跳跃的方式访问容器的任意数据，是功能最强的迭代器；读写，支持++、、[n]、-n、<、<=、>、>=

## 1.3 vector

<div align=center>
    <img src="./images/vector迭代器.png" />
</div>

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

void myPrintInt(int val);

void test01()
{
    // 单端动态数组vector类模板
    std::vector<int> v; // v就是一个具体的vector容器

    // push_back 尾部插入
    v.push_back(100);
    v.push_back(200);
    v.push_back(300);
    v.push_back(400);

    // 访问数据
    // 定义一个迭代器存储v的起始迭代器
    std::vector<int>::iterator it = v.begin();
    // 定义一个迭代器存储v的结束迭代器
    std::vector<int>::iterator endIt = v.end();

    // for循环遍历
    for (; it != endIt; it++) {
        // 对迭代器取* 代表的是容器的元素
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // for循环遍历2（推荐）
    for (std::vector<int>::iterator it = v.begin(); it != v.end(); it++) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // STL提供的算法来遍历容器
    // 包含算法头文件 algorithm
    // for_each 从容器的起始 ---> 结束，逐个元素去取出
    std::for_each(v.begin(), v.end(), myPrintInt);
    std::cout << std::endl;
}

void myPrintInt(int val)
{
    std::cout << val << " ";
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}
```

<b>运行结果：</b>

```bash
100 200 300 400 
100 200 300 400 
100 200 300 400
```

### 1.3.1 for_each 迭代器

<div align=center>
    <img src="./images/for_each迭代器.png" />
</div>

### 1.3.2 存放自定义数据

```cpp
class Person
{
public:
    std::string name;
    int age;
    Person(std::string name, int age) {
        this->age = age;
        this->name = name;
    }
};
void myPrintPerson(Person &ob)
{
    std::cout << "name = " << ob.name << ", age = " << ob.age << std::endl;
}
void test02()
{
    Person ob1("德玛", 18);
    Person ob2("小炮", 28);
    Person ob3("小法", 38);
    Person ob4("小李", 24);

    // 定义一个vector容器存放
    std::vector<Person> v;
    v.push_back(ob1);
    v.push_back(ob2);
    v.push_back(ob3);
    v.push_back(ob4);

    for_each(v.begin(), v.end(), myPrintPerson);
}
```

### 1.3.4 容器嵌套容器

```cpp
void test03()
{
    std::vector<int> v1;
    std::vector<int> v2;
    std::vector<int> v3;

    v1.push_back(10);
    v1.push_back(20);
    v1.push_back(30);
    v1.push_back(40);

    v2.push_back(100);
    v2.push_back(200);
    v2.push_back(300);
    v2.push_back(400);

    v3.push_back(1000);
    v3.push_back(2000);
    v3.push_back(3000);
    v3.push_back(4000);

    // 需求：再定义一个vector容器，存放v1,v2,v3
    std::vector<std::vector<int>> v;
    v.push_back(v1);
    v.push_back(v2);
    v.push_back(v3);

    // for 循环遍历
    for (std::vector<std::vector<int>>::iterator it = v.begin(); it != v.end(); it++) {
        for (std::vector<int>::iterator mit = (*it).begin(); mit != (*it).end(); mit++) {
            std::cout << *mit << "\t";
        }
        std::cout << std::endl;
    }
}
```

# 二、常用容器

## 2.1. string

C风格字符串（以空字符结尾的字符数组）太过复杂难于掌握，不适合大程序的开发，所以C++标准库定义了一种string类，定在在头文件。

<b>`string`和C风格字符串对比：</b>

1. `char*`是一个指针，`string`是一个类
2. `string`封装了`char*`，管理这个字符串，是一个char型的容器。
3. `string`封装了很多实用的成员方法：查找find、拷贝copy、删除delete、替换replace、插入insert
4. 不用考虑内存释放和越界
5. `string`管理`char*`所分配的内存，每一次`string`的赋值，取值都由`string`类负责维护，不用担心复制越界和取值越界等。

## 2.2 string容器常用操作

### 2.2.1 `string`构造函数

```cpp
string();   // 创建一个空的字符串，例如：string str;
string(const string &str);  // 使用一个string对象初始化另一个string对象
string(const char* s);  // 使用字符串s初始化
string(int n, char c);  // 使用n个字符串c初始化v
```

### 2.2.2 `string`基本赋值操作

```cpp
string& operator=(const char *s);   // char*类型字符串赋值给当前的字符串
string& operator=(const string &s); // 把字符串s赋给当前的字符串
string& operator=(char c);  // 字符赋值给当前的字符串
string& assign(const char *s);  // 把字符串s赋给当前的字符串
string& assign(const char *s, int n);   // 把字符串s的前n个字符赋给当前的字符串
string& assign(const string &s);    // 把字符串s赋给当前字符串
string& assign(int n, char c);  // 用n个字符c赋给当前字符串
string& assign(const string &s, int start, int n);  // 将s从start开始n个
```

```cpp
int main(int argc, char **argv)
{
    // string(const char* s);  // 使用字符串s初始化
    string str1("hello string");
    std::cout << str1 << std::endl;

    // string(int n, char c);  // 使用n个字符串c初始化v
    string str2(10, 'a');
    std::cout << str2 << std::endl;

    string str3 = str2;
    std::cout << str3 << std::endl;

    string str4;
    // string& operator=(const string &s); // 把字符串s赋给当前的字符串
    str4 = str1;
    std::cout << str4 << std::endl;

    string str5;
    // string& operator=(const char *s);   // char*类型字符串赋值给当前的字符串
    str5 = "hello str5";
    std::cout << str5 << std::endl;

    // string& operator=(char c);  // 字符赋值给当前的字符串
    string str6;
    str6 = 'h';
    std::cout << str6 << std::endl;

    // string& assign(const char *s);  // 把字符串s赋给当前的字符串
    string str7;
    str7.assign("hello str7");
    std::cout << str7 << std::endl;

    // string& assign(const char *s, int n);   // 把字符串s的前n个字符赋给当前的字符串
    string str8;
    str8.assign("hello str8", 5);
    std::cout << str8 << std::endl;

    // string& assign(const string &s);    // 把字符串s赋给当前字符串
    string str9;
    str9.assign(str8);
    std::cout << str9 << std::endl;

    // string& assign(int n, char c);  // 用n个字符c赋给当前字符串
    string str10;
    str10.assign(10, 'h');
    std::cout << str10 << std::endl;

    // string& assign(const string &s, int start, int n);  // 将s从start开始n个
    string str11;
    str11.assign(str7, 2, 5);
    std::cout << str11 << std::endl;

    return 0;
}
```

#### 2.2.2.1 字符串的存取

```cpp
char& operator[](int n);    // 通过[]方式取字符
char& at(int n);    // 通过at方法获取字符
```

```cpp
void test02()
{
    string str1 = "hello string";
    cout << str1[1] << endl;
    cout << str1.at(1) << endl;

    str1[1] = 'E';
    cout << str1 << endl;
    str1.at(7) = 'T';
    cout << str1 << endl;

    // [] 和 at的区别
    try
    {
        // str1[1000] = 'G';   // 未捕获到异常
        str1.at(1000) = 'G';    // basic_string::at: __n (which is 1000) >= this->size() (which is 12)
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}
```

> [] 和 at() 方法存在一些差异：如果访问越界，使用at()方法会抛出异常，而使用[]则不会抛出异常。

### 2.2.3 字符串拼接操作

```cpp
string& operator+=(const string& str); // 重载+=操作符
string& operator+=(const char* str); // 重载+=操作符
string& operator+=(const char c); // 重载+=操作符
string& append(const char *s); // 把字符串s连接到当前字符串结尾
string& append(const char *s, int n); // 把字符串s的前n个字符连接到当前字符串结尾
string& append(const string &s); // 同operator+=()
string& append(const string &s, int pos, int n); // 把字符串s中从pos开始的n个字符连接到当前字符串结尾
string& append(int n, char c); // 在当前字符串结尾添加n个字符c
```

```cpp
void test03()
{
    // string& operator+=(const string& str); // 重载+=操作符
    string str1 = "hello";
    string str2 = " string";
    str1 += str2;   // str1 = str1 + str2;
    cout << str1 << endl;   // hello string

    // string& operator+=(const char* str); // 重载+=操作符
    string str3 = "hello";
    str3 += " string3";
    cout << str3 << endl;   // hello string3

    // string& operator+=(const char c); // 重载+=操作符
    string str4 = "hello";
    str4 += 'T';
    cout << str4 << endl;   // helloT

    // string& append(const char *s); // 把字符串s连接到当前字符串结尾
    string str5 = "hello";
    str5.append(" str5");
    cout << str5 << endl;   // hello str5

    // string& append(const char *s, int n); // 把字符串s的前n个字符连接到当前字符串结尾
    string str6 = "hello";
    str6.append("hahahahehe", 6);
    cout << str6 << endl;   // hellohahaha

    // string& append(const string &s); // 同operator+=()
    string str7 = "hello";
    string str8 = " str8";
    str7.append(str8);
    cout << str7 << endl;   // hello str8

    // string& append(const string &s, int pos, int n);// 把字符串s中从pos开始的n个字符连接到当前字符串结尾
    string str9 = "hello";
    string str10 = " string10";
    str9.append(str10, 5, 14);  // hellong10
    cout << str9 << endl;

    // string& append(int n, char c); // 在当前字符串结尾添加n个字符c
    string str11 = "hello";
    str11.append(10, 'c');
    cout << str11 << endl;  // hellocccccccccc
}
```

### 2.2.4 字符串查找和替换

```cpp
int find(const string& str, int pos = 0) const; // 查找str第一次出现位置，从pos开始查找
int find(const char* s, int pos = 0) const; // 查找s第一次出现位置，从pos开始查找
int find(const char* s, int pos, int n) const; // 从pos位置查找s的前n个字符第一次位置
int find(const char c, int pos = 0) const; // 查找字符c第一次出现位置
int rfind(const string& str, int pos = npos) const; // 查找s最后一次出现位置，从pos开始查找
int rfind(const char* s, int pos = npos) const; // 查找s最后一次出现位置，从pos开始查找
int rfind(const char* s, int pos, int n) const; // 从pos查找s的前n个字符最后一次位置
int rfind(const char c, int pos = 0) const; // 查找字符c最后一次出现位置
string& replace(int pos, int n, const string& str); // 替换从pos开始n个字符为字符串str
string& replace(int pos, int n, const char* s); // 替换从pos开始的n个字符为字符串s
```

```cpp
void test04()
{
    // int find(const string& str, int pos = 0) const; // 查找str第一次出现位置，从pos开始查找
    string str1 = "hehe:haha:xixi:haha:heihei";
    // 从str1中查找haha
    string tmp = "haha";
    cout << str1.find(tmp) << endl; // 5
    cout << str1.find(tmp, 10) << endl; // 15

    // int find(const char* s, int pos = 0) const; // 查找s第一次出现位置，从pos开始查找
    cout << str1.find("hehe") << endl;  // 0
    cout << str1.find("haha", 6) << endl;   // 15

    // int find(const char* s, int pos, int n) const; // 从pos位置查找s的前n个字符第一次位置
    cout << str1.find("xiiii", 0, 2) << endl;    // 10

    // int find(const char c, int pos = 0) const; // 查找字符c第一次出现位置
    cout << str1.find('x', 0) << endl;  // 10

    // int rfind(const string& str, int pos = npos) const; // 查找s最后一次出现位置，从pos开始查找
    cout << str1.rfind(tmp) << endl;    // 15

    // int rfind(const char* s, int pos = npos) const; // 查找s最后一次出现位置，从pos开始查找
    cout << str1.rfind("hehe", 0) << endl; // 0
    
    // int rfind(const char* s, int pos, int n) const; // 从pos查找s的前n个字符最后一次位置
    cout << str1.rfind("hehexxx", 0, 4) << endl; // 0

    // int rfind(const char c, int pos = 0) const; // 查找字符c最后一次出现位置
    cout << str1.rfind('x') << endl;    // 12

    // string& replace(int pos, int n, const string& str); // 替换从pos开始n个字符为字符串str
    string tmp2 = "####";
    cout << str1.replace(5, 4, tmp2) << endl; // hehe:####:xixi:haha:heihei
    
    // string& replace(int pos, int n, const char* s); // 替换从pos开始的n个字符为字符串s
    cout << str1.replace(15, 4, "&&&&") << endl;    // hehe:####:xixi:&&&&:heihei

    // 需求：将字符串中的所有"sex"用***屏蔽
    string str2 = "www.sex.117114.sex.person.77.com";
    int pos = 0;
    while (1)
    {
        pos = str2.find("sex", pos);
        if (pos == string::npos) {
            break;
        } else {
            str2.replace(pos, strlen("sex"), "***");
        }
    }
    cout << str2 << endl;   // www.***.117114.***.person.77.com
}
```

### 2.2.5 字符串比较

```cpp
compare函数在>时返回1，<时返回-1，==时返回0.
比较区分大小写，比较时参考字典顺序，排越前面的越小。
大写的A比小写的a小。

int compare(const string &s) const; // 与字符串s比较
int compare(const char *s); // 与字符串s比较
```

```cpp
void test05()
{
    string str1 = "hehe";
    string str2 = "haha";
    cout << str1.compare(str2) << endl; // 1

    cout << str1.compare("lala") << endl; // -1

    cout << str1.compare("hehe") << endl; // 0
}
```

C语言中字符串比较函数：`strcmp`
C语言中内存比较函数：`memcmp`

### 2.2.6 字符串提取

```cpp
string substr(int pos = 0, int n = npos) const; // 返回由pos开始的n个字符组成的字符串
```

```cpp
void test06()
{
    string str1 = "hehe:haha:xixi:lala:heihei";
    cout << str1.substr(5, 4) << endl;  // haha

    // 案例：将:分割的所有字符串提取出来
    int pos = 0;
    while (1) {
        int ret = str1.find(':', pos);
        if (ret == string::npos) {
            cout << str1.substr(pos, ret - pos) << endl;
            break;
        }
        cout << str1.substr(pos, ret - pos) << endl;
        pos = ret + 1;
    }
}
```

### 2.2.7 字符串的插入删除

```cpp
string& insert(int pos, const char* s); // 插入字符串
string& insert(int pos, const string& str); // 插入字符串
string& insert(int pos, int n, char c); // 在指定位置插入n个字符c
string& erase(int pos, int n = npos); // 删除从pos开始的n个字符
```

```cpp
void test07()
{
    string str1 = "hello world";
    str1.insert(5, "hehe");
    cout << str1 << endl;   // hellohehe world

    cout << str1.erase(5, 4) << endl;   // hello world

    // 清空字符串
    str1.erase(0, str1.size());
    cout << str1 << endl;   // 
}
```

### 2.2.8 string和c风格的字符串转换

```cpp
void test08()
{
    string str1;
    char *str2 = "hello str";

    // 将char* 转成 string (直接完成)
    str1 = str2;
    cout << str1 << endl; // hello str

    string str3 = "hello str3";
    // 不能直接将string转换成char *，必须借助string中的c_str方法完成。
    // char *str4 = str3;  // error: cannot convert ‘std::__cxx11::string’ {aka ‘std::__cxx11::basic_string<char>’} to ‘char*’ in initialization
    char *str4 = const_cast<char *>(str3.c_str());
    cout << str4 << endl;  // hello str3
}
```

## 2.3 vector容器

vector的数据安排以及操作方式，与array非常相似，两者唯一的差别在于空间运动的灵活性。array是静态空间，一旦配置了就不能改变，要换大一点或者小一点的空间，一切琐碎都得有自己完成，首先配置一块新的空间，然后将旧空间的数据搬往新空间，再释放原来的空间。

vector是动态空间，随着元素的加入，它的内部机制会自动扩充空间以容纳新元素。因此vector的运用对于内存的合理利用与运用的灵活性有很大的帮助，我们再也不必害怕空间不足而一开始就要求一个大块头的array了。

vector的实现技术，关键在于其对大小的控制以及重新配置时的数据移动效率，一旦vector旧空间满了，如果客户每新增一个元素，vector内部只是扩充一个元素的空间，实为不智，因为所谓的扩充空间(不论大小)，是“配置新空间-数据移动-释放旧空间”的大工程，时间成本很高，应该加入某种未雨绸缪的考虑。

<div align=center>
    <img src="./images/vector结构.png" />
</div>

<b>vector容器的迭代器：</b>随机访问迭代器
<b>随机访问迭代器：</b>迭代器++可以通过编译就是随机访问迭代器

### 2.3.1 vector的容量和大小

capacity：空间能容纳元素的最大个数
size：空间中实际存放的元素个数

```cpp
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
```

<b>输出：</b>

```bash
容器内存：0, 扩充容器大小为：0
更新了容器内存：0x55aa2de4c2c0, 扩充容器大小为：1
更新了容器内存：0x55aa2de4c2e0, 扩充容器大小为：2
更新了容器内存：0x55aa2de4c2c0, 扩充容器大小为：4
更新了容器内存：0x55aa2de4c300, 扩充容器大小为：8
更新了容器内存：0x55aa2de4c330, 扩充容器大小为：16
更新了容器内存：0x55aa2de4c380, 扩充容器大小为：32
更新了容器内存：0x55aa2de4c410, 扩充容器大小为：64
更新了容器内存：0x55aa2de4c520, 扩充容器大小为：128
v的容量capacity: 128
v的大小size: 100
```

> 容量 >= 元素的个数size \
> 为啥插入100个元素容量却是128？原因：vector的未雨绸缪机制。\
> 只要空间满，就会另寻空间。

### 2.3.2 vector的构造函数

```cpp
vector<T> v; // 采用模板实现类对象，默认构造函数
vector(v.begin(), v.end()); // 将v[begin(), end()]区间中的元素拷贝给本身
vector(n, elem); // 构造函数将n个elem拷贝给本身
vector(const vector &vec); // 拷贝构造函数
```

```cpp
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
```

> 可以使用int数组赋值，int数组的变量名就是第一个元素的地址。

### 2.3.3 vector的赋值语句

```cpp
assign(beg, end); // 将[beg,end)区间中的数据拷贝赋值给本身
assign(n, elem); // 将n个elem拷贝赋值给本身
vector& operator=(const vector &vec); // 重载等号操作符
swap(vec);  // 将vec与自身的元素互换
```

```cpp
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
```

> `vector& operator=(const vector &vec);`是深拷贝。


### 2.3.4 vector大小操作

```cpp
size(); // 返回容器中元素的个数
empty(); // 判断容器是否为空
resize(int num); // 重新指定容器的长度为num，若容器变长，则以默认值填充新位置。
resize(int num, elem); // 重新指定容器的长度为num，若容器变长，则以elem值填充新位置。
capacity(); // 容器的容量
reserve(int len); // 容器预留len个元素长度，预留位置不初始化，元素不可访问
```

```cpp
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
```

> resize作用的容器的大小，不会更改容器的容量。

### 2.3.5 使用swap收缩容器容量

```cpp
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
```

> 操作步骤：1）`v.resize(10);`先resize到10；2）`vector<int>(v).swap(v);`通过swap交换 \
> 原理：resize之后，v的size只剩下10；`vector<int>(v)`是一个匿名对象容器，将v的实际元素赋值给匿名容器，大小为10，容量也为10；`swap(v)`交换，使用匿名对象和v进行交换，交换之后，v代表匿名对象，匿名对象代表原来的v，`swap()`结束后匿名对象会被释放，相当于把原来的v对象释放了。

### 2.3.6 reserve预留空间

```cpp
// reserve(int len); // 容器预留len个元素长度，预留位置不初始化，元素不可访问
```

```cpp
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
```

### 2.3.7 vector数据的存取

```cpp
at(int idx); // 返回索引idx所指的数据，如果idx越界，抛出out_of_range异常。
operator[]; // 返回索引idx所指的数据，越界时，运行直接报错
front(); // 返回容器中的第一个数据元素
back(); // 返回容器中的最后一个数据元素
```

```cpp
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
```

### 2.3.8 vector的插入和删除

```cpp
insert(const_iterator pos, int count, ele); // 迭代器指向位置pos插入count个ele元素
push_back(ele); // 尾部插入元素ele
pop_back(); // 删除最后一个元素
erase(const_iterator start, const_iterator end); // 删除迭代器从start到end之间的元素
erase(const_iterator pos); // 删除迭代器指向的元素
clear(); // 删除容器中所有元素
```

```cpp
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
```

## 2.4 deque容器

## 2.3.1 deque容器基本概念

`vector`容器是单向开口的连续内存空间，deque则是一种双向开口的连续性空间。所谓的双向开口，意思是可以在头尾两端分别做元素的插入和删除操作，当然，vector容器也可以在头尾两端插入元素，但是在其头部操作效率奇差，无法被接受。

