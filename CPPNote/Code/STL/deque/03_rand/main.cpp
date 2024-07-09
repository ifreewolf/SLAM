#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
    // 设置随机数种子，如果种子是固定的，随机数也是固定的
    // time(NULL)获取当前时间，则每次运行，随机数种子都是不一样的
    srand(time(NULL));
    for (int i = 0; i < 10; i++)
    {
        // rand() 函数的返回值就是随机数
        int num = rand();
        cout << num << " ";
    
    }
    cout << endl;

    return 0;
}