#include <iostream>

class Animal{};
class Dog : public Animal{};
class Other{};

// 静态转换static_cast
void test01()
{
    // static_cast作用于基本类型
    char ch = 'a';
    double b = (double)ch;  // 强制转换
    double c = static_cast<double>(ch);
    std::cout << "b = " << b << std::endl;  // 97
    std::cout << "c = " << c << std::endl;  // 97

    // static_cast 作用域自定义数据类型（结构体和类）
    // 有关系的类之间
    // 上行转换
    Animal *p = new Dog;    // 编辑器默认接受
    Animal *p1 = static_cast<Animal*>(new Dog);
    // 下行转换，不安全，容易越界
    // Dog* dog = new Animal;  // error: invalid conversion from ‘Animal*’ to ‘Dog*’ [-fpermissive]
    Dog* dog = (Dog*)(new Animal);  // 使用强制转换
    Dog* dog1 = static_cast<Dog *>(new Animal); // 使用静态转换

    // static_cast不能作用于不相关的类之间转换
    // Animal* p2 = static_cast<Animal *>(new Other);  // error: invalid static_cast from type ‘Other*’ to type ‘Animal*’
    Animal* p3 = (Animal *)(new Other); // 强制转换是可以的，但是内存访问会很不安全，static转换更加安全

}

// dynamic_cast动态转换
void test02()
{
    // 1、dynamic_cast不支持基本类型
    char ch = 'a';
    // double d = dynamic_cast<double>(ch);    //  cannot dynamic_cast ‘ch’ (of type ‘char’) to type ‘double’ (target is not pointer or reference)

    // 2、dynamic_cast 上行转换（父类指针指向子类空间，安全)
    Animal *p1 = dynamic_cast<Animal *>(new Dog);

    // 3、dynamic_cast 下行转换（子类指针指向父类空间，不安全）
    // Dog *p2 = dynamic_cast<Dog *>(new Animal);  // 不支持不安全的转换
    // error: cannot dynamic_cast ‘(Animal*)operator new(1)’ (of type ‘class Animal*’) to type ‘class Dog*’ (source type is not polymorphic)

    // 4、dynamic_cast不支持没有关系的类型转换
    // Animal *p3 = dynamic_cast<Animal *>(new Other); // cannot dynamic_cast ‘(Other*)operator new(1)’ (of type ‘class Other*’) to type ‘class Animal*’ (source type is not polymorphic)
}

// const_cast常量转换
// 常量与变量之间的互转
void test03()
{
    const int *p = nullptr;
    // int *p1 = p;    //  error: invalid conversion from ‘const int*’ to ‘int*’ [-fpermissive]
    int *p1 = (int *)p; // 这样是可以的
    int *p2 = const_cast<int *>(p); // 这个比较安全

    int *p3 = nullptr;
    const int *p4 = p3; // 编译通过
    const int *p5 = (const int *)p1; // 编译通过
    const int *p6 = const_cast<int *>(p2);
    
    // const_cast不支持非指针或引用的转换
    const int a = 100;
    // int b = const_cast<int>(a); //  error: invalid use of const_cast with type ‘int’, which is not a pointer, reference, nor a pointer-to-data-member type

    int data = 100;
    // 常量引用转换成普通引用
    const int &ob = data;
    int &ob2 = const_cast<int &>(ob);
}


// reinterpret_cast重新编译转换
// 这个转换和C语言风格十分类似，什么类型都能转换，非常霸道，强制类型转换
// 不支持基本类型
void test04()
{
    char ch = 'a';
    // double d = reinterpret_cast<double>(ch); //  error: invalid cast from type ‘char’ to type ‘double’

    // reinterpret_cast
    Animal *p1 = reinterpret_cast<Animal *>(new Other);  // 可以编译

    // 上行转换
    Animal *p2 = reinterpret_cast<Animal *>(new Dog);   // 可以编译

    // 下行转换
    Dog *p3 = reinterpret_cast<Dog *>(new Animal);  // 可以编译
}

int main(int argc, char **argv)
{
    test04();
    return 0;
}