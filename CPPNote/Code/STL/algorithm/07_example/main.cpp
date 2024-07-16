#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <deque>
#include <cstring>
#include <numeric>
#include <map>
using namespace std;

class Speaker
{
public:
    string name;
    int score[3];
public:
    Speaker(){}
    Speaker(string name)
    {
        this->name = name;
        memset(score, 0, sizeof(score));
    }
};

void createSpeaker(vector<int> &v, map<int, Speaker> &m)
{
    for (int i = 0; i < 24; i++)
    {
        // 存放选手编号
        v.push_back(100 + i);
        string name = "选手";
        name += 'A' + i;
        m.insert(make_pair(i+100, Speaker(name)));
    }
}

void speech_contest(int epoch, vector<int> &v, map<int, Speaker> &m, vector<int> &v1)
{
    int count = 0;
    // 设计一个<分数，编号>容器
    multimap<int, int, greater<int>> m2; 
    // 选手逐一登台比赛
    for (vector<int>::iterator it = v.begin(); it != v.end(); it++) {
        count++;
        deque<int> d;
        for (int i = 0; i < 10; i++) { // 10个评委打分
            int score = rand() % 41 + 60;
            d.push_back(score);
        }
        // 对d容器排序
        sort(d.begin(), d.end());
        // 去掉最高分，去掉最低分
        d.pop_back();
        d.pop_front();
        // 求取总分数
        int sum = accumulate(d.begin(), d.end(), 0);

        int avg = sum / d.size();
        m[*it].score[epoch - 1] = avg;
        m2.insert(make_pair(avg, *it));

        if (count % 6 == 0) { // 刚好6人，把上面的6人的成绩取3
            // 90 80 70 60 50 40
            int i = 0;
            for (multimap<int, int, greater<int>>::iterator mit = m2.begin(); mit != m2.end() && i < 3; mit++, i++) {
                v1.push_back(mit->second);
            }
            m2.clear(); // 清空
        }
    }
}

/*
18690705637
*/

void printSpeechResult(int epoch, vector<int> &v, map<int, Speaker> &m, vector<int> &v1)
{
    cout << "第" << epoch << "轮比赛成绩如下" << endl;
    int count  = 0;
    int grp = 0;
    for (vector<int>::iterator it = v.begin(); it != v.end(); it++) {
        if (count % 6 == 0) {
            grp = count/6 + 1;
            cout << "第" << grp << "组的成绩如下：" << endl;
        }

        cout << "\t姓名：" << m[*it].name << ", 得分：" << m[*it].score[epoch - 1] << endl;
        
        count++;

        // 每个组的成绩打印完，立马打印晋级名单
        if (count % 6 == 0) {
            cout << "第" << grp << "组晋级名单如下：" << endl;
            int cnt = 0;
            for (vector<int>::iterator vit = v1.begin() + (grp - 1)*3; vit != v1.end() && cnt < 3; vit++, cnt++) {
                cout << "\t姓名：" << m[*vit].name << ", 得分：" << m[*vit].score[epoch-1] << endl;
            }
        }
    }
}

int main(int argc, char **argv)
{
    // 创建24名选手，将选手<编号，选手>放入map容器中，选手编号放vector<>容器中
    vector<int> v; // 选手编号
    map<int, Speaker> m; // <编号，选手>
    createSpeaker(v, m);

    // for_each(m.begin(), m.end(), [](const pair<int, Speaker> &p){
    //     cout << p.first << ", " << p.second.name << endl;
    // });

    // 设置种子
    srand(time(NULL));
    // 第一轮：参加的选手抽签
    random_shuffle(v.begin(), v.end());
    // 进行第一轮比赛
    // 1表示当前轮数，v选手编号，m选手信息，v1晋级容器
    vector<int> v1;
    speech_contest(1, v, m, v1);
    // 打印第一轮比赛结果：所有参与比赛的成绩 晋级的名单
    printSpeechResult(1, v, m, v1);

    cout << "=============================================" << endl;
    // 第二轮：参加的选手抽签
    random_shuffle(v1.begin(), v1.end());
    // 进行第二轮比赛
    // 2表示当前轮数，v选手编号，m选手信息，v1晋级容器
    vector<int> v2;
    speech_contest(2, v1, m, v2);
    // 打印第二轮比赛结果：所有参与比赛的成绩 晋级的名单
    printSpeechResult(2, v1, m, v2);

    cout << "=============================================" << endl;
    // 第三轮：参加的选手抽签
    random_shuffle(v2.begin(), v2.end());
    // 进行第三轮比赛
    // 3表示当前轮数，v选手编号，m选手信息，v1晋级容器
    vector<int> v3;
    speech_contest(3, v2, m, v3);
    // 打印第一轮比赛结果：所有参与比赛的成绩 晋级的名单
    printSpeechResult(3, v2, m, v3);
    return 0;
}