#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

void myTransInt(int val)
{
    
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
    transform(v1.begin(), v2.end(), v2.begin(), myTransInt);

    for_each(v2.begin(), v2.end(), [](int val){ cout << val << " ";});
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}