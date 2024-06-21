#include "MyString.h"

MyString::MyString()
{
    this->str = nullptr;
    this->size = 0;
    std::cout << "无参构造" << std::endl;
}

MyString::MyString(const char *str)
{
    std::cout << "char *有参构造函数" << std::endl;
    // 申请空间
    this->str = new char[strlen(str) + 1];
    // 拷贝字符串
    strcpy(this->str, str);
    
    // 更新size
    this->size = strlen(str);
}

MyString::MyString(const MyString &ob)
{
    std::cout << "char *拷贝构造函数" << std::endl;
    // 申请空间
    this->str = new char[strlen(ob.str) + 1];
    // 拷贝字符串
    strcpy(this->str, ob.str);
    
    // 更新size
    this->size = ob.size;
}

MyString::~MyString()
{
    std::cout << "析构函数" << std::endl;
    if (this->str) {
        delete [] this->str;
        this->str = nullptr;
    }
}

int MyString::Size()
{
    return this->size;
}

char& MyString::operator[](int index)
{
    std::cout << "[]运算符" << std::endl;
    if (index >= this->size || index < 0) {
        std::cerr << "the length of MyString is " << this->size << ", you index is out of range" << std::endl;
        // return '\n';
    } else {
        return this->str[index];
    }
}

MyString& MyString::operator=(const MyString &ob)
{
    std::cout << "=运算符，ob" << std::endl;
    // 将ob.str拷贝到this->str中 
    if (this->str) {
        delete [] this->str;
        this->str = nullptr;
    }
    this->str = new char[ob.size + 1];
    strcpy(this->str, ob.str);
    this->size = ob.size;
    return *this;   // 返回引用，保证可以链式赋值
}


MyString& MyString::operator=(const char* str)
{
    std::cout << "=运算符，const char* str" << std::endl;
    // 将ob.str拷贝到this->str中 
    if (this->str) {
        delete [] this->str;
        this->str = nullptr;
    }
    this->str = new char[strlen(str) + 1];
    strcpy(this->str, str);
    this->size = strlen(str);
    return *this;   // 返回引用，保证可以链式赋值
}


MyString& MyString::operator+(const MyString &ob)
{
    // this 指向的是str5, ob是str6的别名
    int newSize = this->size + ob.size + 1;
    char *tmp_str = new char[newSize];

    // 清空tmp_str所指向的空间
    memset(tmp_str, 0, newSize);

    // 先将this->str拷贝到tmp_str中，然后将ob.str追加到tmp_str中
    strcpy(tmp_str, this->str);
    strcat(tmp_str, ob.str);

    static MyString newString;
    newString = tmp_str;
    // 释放tmp_str指向的临时空间
    if (tmp_str != nullptr) {
        delete [] tmp_str;
        tmp_str = nullptr;
    }
    return newString;
}

MyString& MyString::operator+(const char* str)
{
    // this 指向的是str5,str是字符串常量
    int newSize = this->size + strlen(str) + 1;
    char *tmp_str = new char[newSize];

    // 清空tmp_str所指向的空间
    memset(tmp_str, 0, newSize);

    // 先将this->str拷贝到tmp_str中，然后将str追加到tmp_str中
    strcpy(tmp_str, this->str);
    strcat(tmp_str, str);

    static MyString newString;
    newString = tmp_str;
    // 释放tmp_str指向的临时空间
    if (tmp_str != nullptr) {
        delete [] tmp_str;
        tmp_str = nullptr;
    }
    return newString;
}

bool MyString::operator==(const MyString &ob)
{
    if ((strcmp(this->str, ob.str) == 0) && (this->size == ob.size)) {
        return true;
    }
    return false;
}


bool MyString::operator==(const char* str)
{
    if ((strcmp(this->str, str) == 0) && (this->size == strlen(str))) {
        return true;
    }
    return false;
}


std::ostream& operator<<(std::ostream &out, MyString &ob)
{
    out << ob.str << ", size = " << ob.Size();
    return out;
}

std::istream& operator>>(std::istream &in, MyString &ob)
{
    // 方法一：
    // std::cout << &(ob.str) << std::endl;
    // in >> ob.str;
    // std::cout << &(ob.str) << std::endl;

    // 方法二：
    // 记得将原有的数据清除
    if (ob.str) {
        delete [] ob.str;
        ob.str = nullptr;
    }
    // 获取键盘输入的字符串
    char buf[1024] = "";   // 临时buf
    in >> buf;  // 先得到键盘输入的数据，然后根据buf的实际大小开辟空间
    ob.size = strlen(buf);
    ob.str = new char[strlen(buf) + 1];
    strcpy(ob.str, buf);
    return in;
}