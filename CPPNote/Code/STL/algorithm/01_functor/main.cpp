#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

// 普通函数作为一元谓词
bool greaterThan20(int val)
{
    return val > 20;
}

// 使用仿函数作为一元谓词
class MyGreaterThan20
{
public:
    bool operator()(int val)
    {
        return val > 20;
    }
};
void test01()
{
    vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    v.push_back(40);
    v.push_back(50);

    for_each(v.begin(), v.end(), [](int val){cout << val << " ";}); // 10 20 30 40 50
    cout << endl;

    // 需求：找出第一个大于20的数
    // 方法一：使用普通函数作为一元谓词
    vector<int>::iterator it = find_if(v.begin(), v.end(), greaterThan20);
    if (it != v.end()) {
        cout << "第一个大于20的数为：" << *it << endl;
    }
    // 方法二：使用仿函数作为一元谓词
    it = find_if(v.begin(), v.end(), MyGreaterThan20());
    if (it != v.end()) {
        cout << "第一个大于20的数为：" << *it << endl;
    }
}

// 普通函数作为二元谓词
bool myGreaer(int v1, int v2)
{
    return v1 > v2;
}
// 仿函数作为二元谓词
class MyGreater
{
public:
    bool operator()(int a, int b)
    {
        return a > b;
    }
};
void test02()
{
    vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    v.push_back(40);
    v.push_back(50);

    for_each(v.begin(), v.end(), [](int val){cout << val << " ";}); // 10 20 30 40 50
    cout << endl;

    // 从大到小排序
    // 方法一：使用普通函数作为二元谓词
    // sort(v.begin(), v.end(), myGreaer); 
    // 方法二：使用仿函数作为二元谓词
    sort(v.begin(), v.end(), MyGreater()); 
    for_each(v.begin(), v.end(), [](int val){cout << val << " ";}); // 50 40 30 20 10
    cout << endl;
}

/*
template<class T> T plus<T> // 加法仿函数
template<class T> T minus<T> // 减法仿函数
template<class T> T multiplies<T> // 乘法仿函数
template<class T> T divides<T> // 除法仿函数
template<class T> T modulus<T> // 取模仿函数
template<class T> T negate<T> // 取反仿函数
*/
void test03()
{
    plus<string> p;
    cout << p("qwer", "tyuiop") << endl; // qwertyuiop
    
    minus<int> m;
    cout << m(90, 70) << endl; // 20

    multiplies<float> mp;
    cout << mp(30.9, 20.1) << endl; // 621.09

    divides<float> d;
    cout << d(90, 1.2) << endl; // 75

    modulus<int> mo;
    cout << mo(1000, 7) << endl; // 6

    negate<int> n;
    cout << n(50) << endl; // -50

}

/*
template<class T> bool equal_to<T> // 等于
template<class T> bool not_equal_to<T> // 不等于
template<class T> bool greater<T> // 大于
template<class T> bool greater_equat<T> // 大于等于
template<class T> bool less<T> // 小于
template<class T> bool less_equal<T> // 小于等于
*/
void test04()
{
    vector<int> v;
    srand(time(NULL));
    for (int i = 0; i < 10; i++) {
        v.push_back(rand() % 100);
    }

    for_each(v.begin(), v.end(), [](int val){ cout << val << " ";}); // 37 93 89 57 22 44 39 71 53 62
    cout << endl;

    sort(v.begin(), v.end(), greater<int>());
    for_each(v.begin(), v.end(), [](int val){ cout << val << " ";}); // 93 89 71 62 57 53 44 39 37 22
    cout << endl;

    sort(v.begin(), v.end(), less_equal<int>());
    for_each(v.begin(), v.end(), [](int val){ cout << val << " ";}); // 22 37 39 44 53 57 62 71 89 93
    cout << endl;

    cout << std::boolalpha << equal_to<int>()(10, 10) << endl; // true
    cout << not_equal_to<int>()(30, 40) << endl; // true
}

int main(int argc, char **argv)
{
    test04();
    return 0;
}