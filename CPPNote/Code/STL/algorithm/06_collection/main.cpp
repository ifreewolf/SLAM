#include <iostream>
#include <algorithm>
#include <vector>
#include <iterator>

using namespace std;

/*
    set_intersection算法，求两个set集合的交集
    注意：两个集合必须是有序序列
    @param beg1 容器1开始迭代器
    @param end1 容器1结束迭代器
    @param beg2 容器2开始迭代器
    @param end2 容器2结束迭代器
    @param dest 目标容器开始迭代器
    @return 目标容器的最后一个元素的迭代器地址
set_intersection(iterator beg1, iterator end1, iterator beg2, iterator end2, iterator dest)
*/

/*
    set_union算法，求两个set集合的并集
    注意：两个集合必须是有序序列
    @param beg1 容器1开始迭代器
    @param end1 容器1结束迭代器
    @param beg2 容器2开始迭代器
    @param end2 容器2结束迭代器
    @param dest 目标容器开始迭代器
    @return 目标容器的最后一个元素的迭代器地址
set_union(iterator beg1, iterator end1, iterator beg2, iterator end2, iterator dest)
*/

/*
    set_difference算法，求两个set集合的差集
    注意：两个集合必须是有序序列
    @param beg1 容器1开始迭代器
    @param end1 容器1结束迭代器
    @param beg2 容器2开始迭代器
    @param end2 容器2结束迭代器
    @param dest 目标容器开始迭代器
    @return 目标容器的最后一个元素的迭代器地址
set_difference(iterator beg1, iterator end1, iterator beg2, iterator end2, iterator dest)
*/
void test01()
{
    vector<int> A;
    A.push_back(1);
    A.push_back(3);
    A.push_back(5);
    A.push_back(7);
    A.push_back(9);
    
    vector<int> B;
    B.push_back(7);
    B.push_back(9);
    B.push_back(2);
    B.push_back(6);
    B.push_back(4);

    // 求交集
    vector<int> iv; // 存放交集
    iv.resize(min(A.size(), B.size()));
    vector<int>::iterator it;
    it = set_intersection(A.begin(), A.end(), B.begin(), B.end(), iv.begin());
    // copy(iv.begin(), iv.end(), ostream_iterator<int>(cout, " ")); // 7 9 0 0 0
    // 存在后面的3个0，如何解决？
    // 方法一：不使用resize，而是用reserve.
    copy(iv.begin(), it, ostream_iterator<int>(cout, " ")); // 7 9
    cout << endl;

    vector<int> uv;
    uv.resize(A.size() + B.size());
    it = set_union(A.begin(), A.end(), B.begin(), B.end(), uv.begin());
    copy(uv.begin(), it, ostream_iterator<int>(cout, " ")); // 1 3 5 7 9 2 4 6
    cout << endl;

    vector<int> dv;
    dv.resize(A.size());
    it = set_difference(A.begin(), A.end(), B.begin(), B.end(), dv.begin());
    copy(dv.begin(), it, ostream_iterator<int>(cout, " ")); // 1 3 5
    cout << endl;

    dv.resize(B.size());
    it = set_difference(B.begin(), B.end(), A.begin(), A.end(), dv.begin());
    copy(dv.begin(), it, ostream_iterator<int>(cout, " ")); // 2 4 6
    cout << endl;
}



int main(int argc, char **argv)
{
    test01();
    return 0;
}