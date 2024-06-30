#include <iostream>

void test01()
{
    int data = 0;
    std::cin >> data;
    std::cout << "data = " << data << std::endl;

    char ch;
    std::cin >> ch;
    std::cout << "ch = " << ch << std::endl;

    // 获取一个字符
    char ch1 = '\0';
    ch1 = std::cin.get();    // 获取一个字符
    std::cout << "ch1 = " << ch1 << std::endl;

    char name[128] = "";
    std::cin >> name;   // 遇到空格、回车结束获取
    std::cout << "name = " << name << std::endl;
}

// 获取带空格的字符串
void test02()
{
    char name[128] = "";
    std::cin.getline(name, sizeof(name));   // 可以获取带空格的字符串
    std::cout << "name = " << name << std::endl;

}

// 忽略缓冲区的前n个字符
void test03()
{
    char name[128] = "";
    std::cin.ignore(2); // 忽略前2字节
    std::cin >> name;
    std::cout << "name = " << name << std::endl;
    // hello world
    // name = llo
}

// 放回缓冲区 cin.putback()
void test04()
{
    char ch = 0, ch2 = 0;
    ch = std::cin.get();    // 获取一个字符
    std::cout << "ch = " << ch <<std::endl;

    ch2 = std::cin.get();
    std::cout << "ch2 = " << ch2 << std::endl;

    std::cin.putback(ch);   // 将ch的字符放回缓冲区

    char name[32] = "";
    std::cin >> name;
    std::cout << "name = " << name << std::endl;
    // qwreert
    // ch = q
    // ch2 = w
    // name = qreert
}

// 偷窥 cin.peek
void test05()
{
    char ch = '\0';
    ch = std::cin.peek();
    std::cout << "偷窥缓冲区的数据为：" << ch << std::endl;

    char name[32] = "";
    std::cin >> name;
    std::cout << "name = " << name << std::endl;
}

// 

int main(int argc, char **argv)
{
    // test01();
    // test02();
    // test03();
    // test04();
    test05();

    return 0;
}