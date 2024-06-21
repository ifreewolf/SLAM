#include "MyString.h"

int main(int argc, char **argv)
{
    MyString str1("hehehaha");
    std::cout << str1 << std::endl;

    // 自定义对象 必须重载<< （普通全局函数）
    std::cin >> str1;
    std::cout << str1 << std::endl;

    MyString str2("hello class");
    // 重载[]运算符
    std::cout << str2[10] << std::endl;

    // 重载[]运算符，返回值必须是左值，才能写操作
    // 重载[]运算符，返回值必须是引用
    str2[1] = 'E';
    std::cout << str2 << std::endl;

    MyString str3("hello str3");
    std::cout << str3 << std::endl;

    // 将对象str2赋值给str3
    str3 = str2;    // 默认赋值语句，是浅拷贝。重载等号运算符
    std::cout << str3 << std::endl;

    MyString str4("hello str4");
    std::cout << str4 << std::endl;
    // 重载=运算符
    str4 = "hello string";  // 会调用默认的char*有参构造函数，构造了一个tmp对象，然后再使用=运算符赋值给str4对象。如果在有参构造函数前加上explict关键字之后，这句编译不通过
    std::cout << str4 << std::endl;

    // 字符串拼接
    MyString str5("我爱大家");
    MyString str6("我爱千峰");
    // 重载+运算符, ob
    std::cout << str5 + str6 << std::endl;
    std::cout << str1 + str2 << std::endl;

    MyString str7("大家爱我");
    // 重载+运算符，const char*
    std::cout << str7 + "前锋爱我" << std::endl;

    MyString str8("hehe");
    MyString str9("haha");
    // 重载=运算符
    if (str8 == str9) {
        std::cout << "相等" << std::endl;
    } else {
        std::cout << "不相等" << std::endl;
    }

    if (str8 == "hehe") {
        std::cout << "相等" << std::endl;
    } else {
        std::cout << "不相等" << std::endl;
    }

    std::cout << "====================" << std::endl;

    MyString fgs1 = MyString("abc");    // 调用有参构造函数
    MyString fgs2("abcd");  // 调用有参构造函数
    return 0;
}