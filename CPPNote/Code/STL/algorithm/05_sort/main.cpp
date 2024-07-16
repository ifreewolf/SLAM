#include <iostream>
#include <algorithm>
#include <vector>

using namespace std;

/*
merge
*/
void test01()
{
    vector<int> v1;
    v1.push_back(1);
    v1.push_back(3);
    v1.push_back(5);
    v1.push_back(7);

    vector<int> v2;
    v2.push_back(2);
    v2.push_back(4);
    v2.push_back(6);
    v2.push_back(8);

    vector<int> v3;
    // 预先设置v3的大小
    v3.resize(v1.size() + v2.size());
    merge(v1.begin(), v1.end(), v2.begin(), v2.end(), v3.begin());
    for_each(v3.begin(), v3.end(), [](int val) { cout << val << " "; }); // 1 2 3 4 5 6 7 8
    cout << endl;
}

/*
random_shuffle
*/
void test02()
{
    srand(time(NULL));
    vector<int> v1;
    v1.push_back(1);
    v1.push_back(3);
    v1.push_back(5);
    v1.push_back(7);

    for_each(v1.begin(), v1.end(), [](int val) { cout << val << " "; }); // 1 3 5 7
    cout << endl;

    random_shuffle(v1.begin(), v1.end());

    for_each(v1.begin(), v1.end(), [](int val) { cout << val << " "; }); // 1 7 3 5，每次都是1735，需要设置种子
    cout << endl;
}

/*
reverse(iterator beg, iterator end)
*/
void test03()
{
    srand(time(NULL));
    vector<int> v1;
    v1.push_back(1);
    v1.push_back(3);
    v1.push_back(5);
    v1.push_back(7);

    for_each(v1.begin(), v1.end(), [](int val) { cout << val << " "; }); // 1 3 5 7
    cout << endl;

    reverse(v1.begin(), v1.end());

    for_each(v1.begin(), v1.end(), [](int val) { cout << val << " "; }); // 7 5 3 1
    cout << endl;
}

/*
copy
*/
#include <iterator>
void test04()
{
    vector<int> v1;
    v1.push_back(1);
    v1.push_back(3);
    v1.push_back(5);
    v1.push_back(7);

    for_each(v1.begin(), v1.end(), [](int val) { cout << val << " "; }); // 1 3 5 7
    cout << endl;

    vector<int> v2;
    // 预先设置大小
    v2.resize(v1.size());
    copy(v1.begin(), v1.end(), v2.begin());

    for_each(v2.begin(), v2.end(), [](int val) { cout << val << " "; }); // 1 3 5 7
    cout << endl;

    // copy秀一下：用copy输出
    copy(v2.begin(), v2.end(), ostream_iterator<int>(cout, " ")); // 1 3 5 7
}

/*
replace
*/
void test05()
{
    vector<int> v1;
    v1.push_back(1);
    v1.push_back(3);
    v1.push_back(5);
    v1.push_back(7);

    replace(v1.begin(), v1.end(), 3, 3000);

    copy(v1.begin(), v1.end(), ostream_iterator<int>(cout, " ")); // 1 3000 5 7
    cout << endl;
}

/*
    replace_if算法，将容器内指定范围满足条件的元素替换为新元素
    @param beg 容器开始迭代器
    @param end 容器结束迭代器
    @param callback 函数回调或者谓词(返回bool类型的函数对象)
    @praram newvalue 新元素
*/
void test06()
{
    vector<int> v1;
    v1.push_back(1);
    v1.push_back(3);
    v1.push_back(5);
    v1.push_back(7);

    // 将容器中大于3替换成3000
    replace_if(v1.begin(), v1.end(), bind2nd(greater<int>(), 3), 3000);

    for_each(v1.begin(), v1.end(), [](int val) { cout << val << " "; }); // 1 3 3000 3000
    cout << endl;
}

/*
accumulate
*/
#include <numeric>
void test07()
{
    vector<int> v1;
    v1.push_back(1);
    v1.push_back(3);
    v1.push_back(5);
    v1.push_back(7);

    int sum = accumulate(v1.begin(), v1.end(), 0);
    cout << "sum = " << sum << endl; // sum = 16
}

/*
    fill算法，向容器中添加元素
    @param beg 容器开始迭代器
    @param end 容器结束迭代器
    @param value 填充元素
*/
void test08()
{
    vector<int> v1;
    v1.resize(10);
    // v1.reserve(10); // reserve不行，因为reserve是预留空间，但未开辟内存，此时begin()==end()
    fill(v1.begin(), v1.end(), 100);
    copy(v1.begin(), v1.end(), ostream_iterator<int>(cout, " "));
    cout << endl;
}

int main(int argc, char **argv)
{
    test08();
    return 0;
}