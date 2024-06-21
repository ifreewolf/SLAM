#ifndef MYSTRING_H
#define MYSTRING_H

#include <iostream>
#include <string.h>
#include <ostream>

class MyString
{
    friend std::ostream& operator<<(std::ostream &out, MyString &ob);
    friend std::istream& operator>>(std::istream &in, MyString &ob);
private:
    char *str;
    int size;
public:
    MyString();
    explicit MyString(const char *str);
    MyString(const MyString &ob);
    ~MyString();
    int Size(void);

    // 重载[]
    char& operator[](int index); // 如果返回值是char，则只能读，不能写。因为char返回的是具体值，是一个右值，必须返回一个引用
    // 重载=，参数是对象
    MyString& operator=(const MyString &ob);
    // 重载=，参数是字符串常量 const char*
    MyString& operator=(const char* str);
    // 重载+，参数是对象
    MyString& operator+(const MyString &ob);
    // 重载+，参数是字符串常量，const char*
    MyString& operator+(const char* str);
    // 重载==运算符，参数是对象
    bool operator==(const MyString &ob);
    // 重载==运算符，参数是字符串常量
    bool operator==(const char* str);
};

#endif