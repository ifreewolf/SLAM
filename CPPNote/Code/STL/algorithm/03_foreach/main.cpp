#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int myTransInt(int val)
{
    return val;
}

int myTransAdd(int val, int tmp)
{
    return val + tmp;
}

void test01()
{
    vector<int> v1;
    v1.push_back(10);
    v1.push_back(20);
    v1.push_back(30);
    v1.push_back(40);
    v1.push_back(50);

    // 将v1容器的元素搬运到v2中
    vector<int> v2;
    // 预先：设置v2的大小
    v2.resize(v1.size());
    transform(v1.begin(), v1.end(), v2.begin(), myTransInt);

    for_each(v2.begin(), v2.end(), [](int val){ cout << val << " ";}); // 10 20 30 40 50
    cout << endl;

    vector<int> v3(v1.size(), 0);
    transform(v1.begin(), v1.end(), v2.begin(), bind2nd(ptr_fun(myTransAdd), 100));
    for_each(v2.begin(), v2.end(), [](int val){ cout << val << " ";}); // 110 120 130 140 150
    cout << endl;
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}