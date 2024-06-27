#include "person.h"
// 但是include一般不会引入cpp文件
#include "person.cpp"
#include <string>

int main(int argc, char **argv)
{
    // 类模板会经过两次编译，第一次是类模板本身编译(这一步没问题)，第二次是类模板的成员函数调用的时候会再次编译
    // C++/C 独立文件编译
    // 由于本文件只引入了person.h文件，所以在编译下面的对象时，会把参数替换person.h的T1和T2。person.cpp文件没有导入，所以函数的实现没法替换，所以会报错：undefined reference
    // 如果类模板的.cpp和.h分文件，出错的原因在第二次。
    // 如果#include "person.cpp"也引入进来，则可以编译通过。
    // 建议.cpp和.h放在一个文件下
    Person<std::string, int> ob1("德玛西亚", 18);   // undefined reference to `Person<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int>::Person(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int)'

    // ob1.showPerson();

    return 0;
}