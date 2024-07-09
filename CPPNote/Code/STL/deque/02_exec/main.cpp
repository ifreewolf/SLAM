#include <iostream>
#include <deque>
#include <vector>
#include <string>
#include <time.h>
#include <algorithm>
#include <numeric>

using namespace std;

class Person
{
public:
    string name;
    float score;
    Person(string name, float score)
    {
        this->name = name;
        this->score = score;
    }
};

void createPerson(vector<Person> &v)
{
    // 5名选手，ABCDE
    string nameTemp = "ABCDE";
    for (int i = 0; i < 5; i++) {
        string name = "选手:";
        name += nameTemp[i];

        // 将选手的姓名 分数0 放入vector容器中
        v.push_back(Person(name, 0.0));
    }
}

void printVectorPerson(vector<Person> &v)
{
    for (vector<Person>::iterator it = v.begin(); it != v.end(); it++) {
        cout << (*it).name << ", " << (*it).score << endl;
    }
}

void palyGame(vector<Person> &v)
{
    // 设计随机种子
    srand(time(NULL));
    // 容器中的每个人逐一参加比赛
    for (vector<Person>::iterator it = v.begin(); it != v.end(); it++) {
        // 每位选手都要被10个评委打分
        deque<int> d;
        for (int i = 0; i < 10; i++) { // 10个评委
            int score = rand() % 41 + 60; // 60 ~ 100
            d.push_back(score);
        }

        // 对deque容器（评委的10个分数）排序
        sort(d.begin(), d.end());

        for (deque<int>::iterator mit = d.begin(); mit != d.end(); mit++) {
            cout << *mit << " ";
        }
        cout << endl;

        // 去掉一个最低分，一个最高分
        d.pop_front();
        d.pop_back();
        
        // 得到总分数
        int sum = accumulate(d.begin(), d.end(), 0);
        // 获取平均分，选手的score        
        it->score = (float)sum / d.size();
    }
}

int main(int argc, char **argv)
{
    // 1. 定义一个vector容器存放Person
    vector<Person> v;
    createPerson(v);

    // 2. 遍历vector容器
    printVectorPerson(v);

    // 2. 5名选手逐一参加比赛
    palyGame(v);

    // 3. 
    printVectorPerson(v);

    return 0;
}
