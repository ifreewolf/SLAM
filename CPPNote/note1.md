封装：把客观的事务抽象成一个类（将数据和方法打包在一起，加以权限的区分，达到保护并安全使用的目的）。

继承：类之间的关系，这种关系使得对象可以继承另外一类对象的特征和能力，目的是：避免公用代码的重复开发，减少代码和数据冗余。

多态：一个接口，多种方法，程序在运行时才决定调用的函数。

# c++语法检查增强

C语言是弱语法语言，C++是强语法语言。

## 全局变量监测增强
c语言代码：
```cpp
int a = 10; // 有赋值，当作定义
int a; // 同名，没有赋值，当作声明

int main()
{
    printf("a：%d\n", a);
    return 0;
}
```
上面代码在c语言中是可以编译通过的，但在C++中编译失败，会报重复定义的错误。

## C++中所有的变量和函数都必须有类型

C语言代码：

```cpp
// i 没有写类型，可以是任意类型
int fun1(i) {
    printf("%d\n", i);
    return 0;
}

// i 没有写类型，可以是任意类型
int fun2(i) {
    printf("%s\n", i);
    return 0;
}

// 没有写参数，代表可以传任何类型的实参
int fun3()
{
    printf("fun33333333\n");
    return 0;
}

fun1(2);
fun2("string");

// 在C语言中，上述代码可以编译通过，C++不行。

// C语言中，如果函数没有参数，建议写void，代表没有参数
```

## 更严格的类型转换

在C++，不同类型的变量一般是不能直接赋值的，需要相应的强转。C语言代码：

```cpp
typedef enum COLOR { GREEN, RED, YELLOW } color;

int main()
{
    color myColor = GREEN;
    mycolor = 10;
    // 在C语言中，枚举类型在编译中中就是int类型。
    printf("mycolor: %d", mycolor);
    char *p = malloc(10);
    return 0;
}
```

> C代码C编译器可通过，C++编译器无法编译通过。

## struct 类型加强

c中定义结构体变量需要加上struct关键字，C++不需要。c中的结构体只能定义成员变量，不能定义成员函数。C++既可以定义成员变量，也可以定义成员函数。

c语言代码：
```c
#include <stdio.h>
struct stu
{
    int num;
    char name[32];
};

void test01()
{
    // stu lucy = { 100, "lucy" }; // 会报错，unknown type name "stu"
    struct stu lucy = { 100, "lucy" };  // 编译通过
}
```

> c++代码可以不需要使用struct关键字。
> 1. C++的结构体中既可以定义成员变量，也可以定义成员函数

## 新增“bool”类型关键字

标准C++的bool类型有两种内建的常量true(转换为整数1)和false(转换为整数0)表示状态。这三个名字都是关键字。bool类型只有两个值，占1个字节大小，给bool类型赋值时，非0值都会自动转换为true，0值会自动转换为false。

C语言中也有bool类型，在C99标准之前是没有bool关键字，C99标准已经有bool类型，包含头文件stdbool.h就可以使用和C++一样的bool类型。

## 三目运算符功能增强

C语言三目运算表达式返回值为数据值，为右值，不能赋值。

```c
int a = 10;
int b = 20;
printf("ret:%d\n", a > b ? a : b);
// 思考一个问题，(a > b ? a : b) 三目运算符表达式返回的是什么？

// (a > b ? a : b) = 100;
// 返回的是右值
```

C++语言三目运算表达式返回值为变量本身(引用)，为左值，可以赋值。

```cpp
a > b ? a : b = 100;    // 返回的是左值，变量的引用
```

> [左值和右值概念]在C++中可以放在赋值操作符左边的是左值，可以放在赋值操作符右边的是右值。有些变量既可以当左值，也可以当右值。左值为Lvalue，L代表Location，表示<B>内存可以寻址</B>，可以赋值。右值为Rvalue，R代表Read，就可以知道它的值。比如：int temp = 10; temp在内存中有地址，10没有，但是可以Read到它的值。

# C/C++中的const

const是C/C++中的一个关键字，是一个限定符，它用来限定一个变量不允许改变，它将一个对象转换成一个常量。

```cpp
const int a = 10;
a = 100; // 编译错误，const是一个常量，不可修改
```

## C和C++中const的区别

### C中的const

C中的const本质是变量，只读变量，既然是变量那么就会给const分配内存，并且在C中const是一个只读变量，C语言中const修饰的只读变量是外部连接的。

外部连接：其他源文件也可以使用。

```cpp
const int arrSize = 10;
int arr[arrSize];   // 在C语言中，这是错误的。因为arrSize占用某块内存，所以C编译器不知道它在编译时的值是多少。
```

> 如果知道arrSize的地址，可以通过地址间接的修改num的值。

```c
int *p = (int *)&arrSize;
*p = 2000;
printf("num = %d\n", num);  // 此时结果为2000
```

> 局部只读变量(局部const变量)，内存在栈区(可读可写) \
> 全局只读变量，内存在文本常量区，不可修改。\
> 所以，如果arrSize是在函数内定义，可以间接修改；但如果arrSize是在全局定义的，则无法对其进行修改。

总结：在C语言中
1. const修饰全局变量，变量名只读，内存空间在文字常量区(只读)、不能通过变量的地址修改空间内容；
2. const修饰局部变量，变量名只读，内存空间在栈区(可读可写)，可以通过data地址间接的修改空间内容。

### C++中的const

出现在所有函数之外的const作用于当前文件(其他文件不可见)，默认为内部连接(只在当前源文件有效)。

如果必须用在其他源文件 使用只读的全局变量 必须加extern转换成外部连接。

```cpp
extern const int num = 100; // 声明+定义，边声明为外部连接
// 然后在其他文件中就可以使用如下：
extern const int num;   // 声明
```

出现在函数内部(局部)const变量，

```cpp
const int data = 10;
// data = 100;     // err 只读
int *p = (int *)&data;
*p = 2000;
std::cout << "*p = " << *p  << std::endl;   // *p = 2000
std::cout << "data = " << data << std::endl;    // data = 10, 未发生修改
```

1. 对于基础数据类型，也就是const int a = 10这种，编译器会把它放到符号表中，不分配内存，当对其取地址时，会分配内存。

> data在符号表中，当对data取地址，这个时候为data分配了新的空间，*p操作的是分配的空间，而data是从符号表获取的值。

const在C++中使用符号表的形式，更类似于`#define`的用法。

2. 当以变量的形式初始化 const 修饰的变量，系统会为其开辟空间。

```cpp
int b = 200;
const int a = b;    // 系统直接为a开辟空间，而不会把a放入符号表中。
int *p = (int *)&a;
*p = 3000;
std::cout << "*p = " << *p << std::endl;    // 3000
std::cout << "a = " << a << std::endl;      // 3000
```

const内容能不能修改，取决于cont变量是否放入了符号表中，放入了符号表则无法修改。

3. 对于自定义数据类型(结构体、对象)，比如类对象，那么也会分配内存。

```cpp
const Person person = { 100， "lucy"};    // 未初始化age
// person.age = 50;     // 不可修改
std::cout << "age: " << person.age << std::endl;
Person *person = (Person *)&person;
// 指针间接修改
pPerson->age = 150;
std::cout << "age: " << person.age << std::endl;    // 150
pPerson->age = 200；
std::cout << "age: " << person.age << std::endl;    // 200
```

### 尽量以const替换`#define`

```c
#define MAX 1024;
```
`#define`从未被编译器看到过，因为在预处理阶段所有的MAX已经被替换为1024，于是MAX并没有将其加入到符号表中。

如果使用常量获得一个编译错误信息时，可能会带来一些困惑，因为这个信息可能会提到1024，但是并没有提到MAX，如果MAX被定义在一个不是你写的头文件中，可能并不知道1024代表什么，也许解决这个问题要花费很长时间。解决方法就是用一个常量替换上面的宏。

```cpp
const int max = 1024;
```

#### const 和 define区别

1. const有类型，可进行编译器类型安全检查。#define 无类型，不可进行类型检查。
2. const有作用域，而 #define 不重视作用域，默认定义处到文件结尾处，如果定义在指定作用域下有效的常量，那么#define就不能用。

```cpp
#define MAX 1024
const short myMax = 1024;

void func(short i)
{
    std::cout << "short函数" << std::endl;
}

void func(int i)
{
    std::cout << "int 函数" << std::endl;
}

void test()
{
    func(MAX);  // int函数， MAX默认是使用int类型强转，数字默认是整型
    func(myMax);    // short函数
}
```

```cpp
void func1(void)
{
    // 作用范围 是当前复合语句
    const int my_num = 10;

    // 作用范围 当前位置到文件结束
    #define MY_NUM 10;
}

void fun2(void)
{
    // cout << "my_num = " << my_num << std::endl; // err, 不识别
    std::cout << "MY_NUM = " << MY_NUM << std::endl;    // 10
}

// #define的作用域是从定义处开始到文件结尾处。
```

> 命名空间对宏常量无法限制作用范围。即使是在命名空间内定义宏常量，也不属于命名空间，而是属于文件。


# 引用(reference)

> 能用引用，绝对不用指针。

变量名本质是一段连续空间的别名，是一个标号，程序中通过变量来申请并命名内存空间，通过变量的名字可以使用内存空间。

引用可以作为一个已经定义好的变量的别名，基本语法：`Type& ref = val;` &在这里不是取地址，而是起标识作用。

类型标识符是指目标变量的类型，必须在声明引用变量时进行初始化。<b>引用初始化之后不能改变</b>。不能有NULl引用。必须确保引用是和一块合法的存储单元关联。可以建立对数组的引用。

有点类似于：`Type* const ref = &val;`不能修改ref指针的指向。

> 引用的核心：给已有的变量起个别名，不会开辟空间。

上面ref本身不存在空间。ref完全等价于val。

注意：

1. 引用必须初始化
2. 引用一旦初始化，就不能再次修改别名

```cpp
int num = 10;
int &a = num;

int data = 20;
a = data;   // 不是data别名为a，而是将data的值赋给a这个别名所指向的变量。

// a是num的别名，所以a和num具有相同的地址空间。
std::cout << "a的地址空间：" << &a << std::endl;        // 0x61fe88
std::cout << "num 的地址空间：" << &num << std::endl;   // 0x61fe88
```

## 引用作用于数组

引用给数组起个别名

1. 方式一：梁哥法

```cpp
void test01()
{
    int arr[5] = { 10, 20, 30, 40, 50 };
    // 需求：给arr起个别名
    int (&my_arr)[5] = arr;  // my_arr就是数组arr的别名
    int i = 0;
    for (int i = 0; i < 5; i++) {
        cout << my_arr[i] << " ";
    }
    std::cout << std::endl;
}
```

2. 方式二：配合typedef

```cpp
void test02()
{
    int arr[5] = { 10, 20, 30, 40, 50 };
    // 需求：给arr起个别名
    // 1、用typedef给数组取个别名
    // TYPE_ARR 就是一个数组类型（有5个元素，每个元素为int）
    typedef int TYPE_ARR{5};

    // TYPE_ARR new_arr = { 1, 2, 3, 4, 5 };
    TYPE_ARR &myArr = arr;

    int i = 0;
    for (int i = 0; i < 5; i++) {
        cout << my_arr[i] << " ";
    }
    std::cout << std::endl;
}
```

## 函数中的引用

在函数参数和返回值中使用引用，当引用被用作函数参数时，在函数内对任何引用的修改，将对函数外的参数产生改变。如果从函数中返回一个引用，必须像从函数中返回一个指针一样对待。当函数返回值时，引用关联的内存一定要存在。

### 在函数参数中使用引用

通过引用参数产生的效果同按地址传递是一样的，引用的语法更清楚简单。

1) 函数调用时传递的实参不必加"&"符
2) 在被调用函数中不必在参数前加"*"符，引用作为其他变量的别名而存在，因此在一些场合可以代替指针。C++主张用引用传递取代地址传递的方式，因为引用语法容易且不易出错。

### 在函数返回值中使用引用

给函数的返回值起一个别名

```cpp
// 引用作为函数的返回值类型
int& my_data()
{
    int num = 100;
    return num; // 函数返回啥变量，引用就是该变量的别名
}

void test05()
{
    // ret是别名，是函数返回值num的别名
    int &ret = my_data();   // 非法访问
}

// 函数内局部变量在退出函数后，num会被释放。
// 函数的返回值是引用时，不要返回局部变量
int& my_data1()
{
    static int num = 100;
    return num; // 函数返回啥变量，引用就是该变量的别名
}

void test05()
{
    // ret是别名，是函数返回值num的别名
    int &ret = my_data();   // 合法，static的生存周期是整个生命周期
}
```

> 不能返回局部变量的引用。
> 函数返回值当左值，那么函数的返回值类型必须是引用。

```cpp
int& my_data(void)
{
    static int num = 10;
    std::cout << "num = " << num << std::endl;
    return num;
}

void test06()
{
    // 函数的返回值 作为左值
    my_data() = 2000;   // num = 10 等价于num = 2000
    my_data();  // num = 2000
}
```

### 引用的本质

引用的本质在C++内部实现是一个<b>指针常量</b>`Type& ref = val; // Type* const ref = &val;`

C++编译器在编译过程中使用常指针作为引用的内部实现，因此引用所占用的空间大小与指针相同，只是这个过程是编译器内部实现，用户不可见。

> 指针常量：int *const p;
> 常量指针：const int* p;

### 指针引用

```cpp
void fun(int **);
给指针变量取一个别名
Type* pointer = NULL;
Type* &newPointer = pointer;

#include <stdlib.h>
#include <string.h>
void my_str1(char **p_str)  // p_str = &str;
{
    // *p_str == *&str ==  str
    *p_str = (char *)calloc(1, 32);
    strcpy(*p_str, "hello world");
}

void my_str2(char* &my_str)
{
    my_str = (char *)calloc(1, 32); // 1块 32字节
    strcpy(my_str, "hello world");
}

void test07()
{
    char *str = NULL;
    // 需求：封装一个函数从堆区给str申请一个空间，并赋值为"hello world"
    my_str1(&str);
    cout << "str = " << str << endl;    // str = hello world
    my_str2(str);
    cout << "str = " << str << endl;    // str = hello world
}
```

### 常量引用


```cpp
typedef struct
{
    int num;
    char name[32];
}STU;

void myPrintSTU1(STU tmp)   // 普通结构体变量作为形参 开销太大 36个字节
{
    cout << sizeof(tmp) << endl;    // 36
    cout << "学号： " << tmp.num << ", 姓名：" << tmp.name << std::endl;    // 学号：100, 姓名：lucy
}

void myPrintSTU2(STU &tmp)   // tmp是lucy的别名 tmp没有开辟独立空间
{
    cout << sizeof(tmp) << endl;    // 36，这里tmp是引用，本质是指针常量，C++编译器内部在使用tmp是，默认会对其进行解地址(解lucy)，所以结果仍然是36，但实际上tmp没有开辟新的内存。
    tmp.num = 2000; // 引用可以进行修改，引用可能导致实参随形参改变而改变
    cout << "学号： " << tmp.num << ", 姓名：" << tmp.name << std::endl;    // 学号：2000, 姓名：lucy
}

void myPrintSTU3(const STU &tmp)    // 常量引用，防止对引用的内容进行修改
{
    cout << sizeof(tmp) << endl;
    // tmp.num = 200;  // err, 因为tmp为常量引用
    cout << "学号： " << tmp.num << ", 姓名：" << tmp.name << std::endl;    // 学号：2000, 姓名：lucy
}

void test08()
{
    STU lucy = {100, "lucy"};

    // 需求：定义一个函数，打印lucy成员
    myPrintSTU1(lucy);
    myPrintSTU2(lucy);
    myPrintSTU3(lucy);
}
```

### 常量的引用

```cpp
void test09()
{
    // 给常量10取个别名 叫num
    // int &针对的是int，10是const int类型
    int &num = 10;  // err, 无法使用右值给int&赋值
    // const int针对的是const int，10就是const int
    const int &num = 10;    // 这是可以的
}
```

# 宏函数与内联函数的比较

内联函数（inline）

在c中，一些短并且执行频繁的计算写成宏，而不是函数。这样可以获得更高的执行效率，宏可以避免函数调用的开销，这些都是在预处理中完成的。

C++中，使用宏函数出现的问题：1）宏看起来像是一个函数调用，但是会隐藏一些难以发现的错误。2）预处理不允许访问类的成员，也就是说预处理宏不能作为类的成员函数。

为了保持预处理宏的效率又增加安全性，而且还能像一般成员函数那样可以在类里访问自如，C++引入了内联函数(inline function)。

内联函数为了集成宏函数的效率，<b>没有函数调用时开销</b>，又可以像普通函数那样，可以进行参数、返回值类型的安全检查，又可以作为成员函数。

内联函数本质是一个真正的函数，拥有普通函数的行为，不同之处在于内联函数会在适当的地方像预定义宏一样展开，所以不需要函数调用开销。

内联函数必须函数体和声明结合在一起。

内联函数的确占用空间，但是内联函数相对于普通函数的优势只是省去了函数调用时候的压栈，跳转，返回的开销。内联函数用空间换时间。

## 类内部的内联函数

任何在类内部定义的函数自动成为内联函数。

```cpp
class Person
{
public:
    Person() { std::cout << "构造函数！" << std::endl; }
    void PrintPerson() { std::cout << "输出Person!" << std::endl; }
}

// 以上两个都将自动成为内联函数，目的是提高效率。
```

## 内联函数和编译器

内联函数不是何时何地都有效

<b>函数的替换发生在编译阶段，而不是预处理阶段。</b>

当调用一个内联函数的时候，编译器首先确保传入参数类型是正确匹配的，或者如果类型不完全匹配，但是可以将其转换为正确类型，并且返回值在目标表达式里匹配正确类型，或者可以转换为正确类型，内联函数就会直接替换函数调用，这就消除了函数调用的开销。

加入内联函数是成员函数，对象this指针也会被放入合适位置，类型检查和类型转换、包括在合适位置放入对象this指针这些都是预处理器不能完成的。

C++内联函数编译的一些限制，以下情况编译器不会将函数进行内联编译：

1. 不能存在任何形式的循环语句；
2. 不能存在过多的条件判断语句；
3. 函数体不能过于庞大，不能对函数进行取址操作。

内联函数仅仅只是给编译器一个建议，编译器不一定会接受这种建议，如果你没有将函数声明为内联函数，那么编译器也可能将此函数做内联编译。一个好的编译器将会内联小的、简单的函数。

# 函数的默认参数

C++在<b>声明函数原型时</b>可为一个或者多个参数<b>指定默认(缺省)的参数值</b>，当函数调用的时候如果没有指定这个值，编译器会自动用默认值代替。

```cpp
int my_add(int x = 10, int y = 20)
{
    return x + y;
}

void test02()
{
    std::cout << "my_add = " << my_add(100, 200) << std::endl;  // 300
    std::cout << "my_add = " << my_add(100) << std::endl;  // 120
    std::cout << "my_add = " << my_add() << std::endl;  // 30
}
```

注意点：

1. 函数的默认参数从左到右，如果一个参数设置了默认参数，那么这个参数之后的参数都必须设置默认参数。
2. 如果函数声明和函数定义分开写，函数声明和函数定义不能同时设置默认参数，默认参数只在函数声明处起作用。


## 函数的占位参数

C++在声明函数时，可以设置占位参数。占位参数只有参数类型声明，没有参数名。一般情况下，在函数体内部无法使用占位参数。

由于有类型名，所以函数调用的时候，必须给占位参数传参； \
由于没有形参名，所以函数内部是无法使用占位参数的。

```cpp
void func3(int x, int y, int)
{
    std::cout << "x = " << x << ", y = " << y << std::endl;
    return;
}

void test05()
{
    func3(10, 30, 40);
}
```

> 操作符重载的后置++要用到这个。


# 函数重载(overload)

C++多态的特性

<b>函数重载意义</b>：方便的使用函数名

函数重载条件：同一个作用域，参数个数不同/参数类型不同/参数顺序不同 均可重载

<b>注意</b>

1. 函数的返回值类型不能作为重载的依据；
   在编写程序过程中可以忽略他的返回值。
2. 函数重载和默认参数一起使用，需要额外注意二义性问题的产生。
   ```cpp
    void myFunc02(int a)
    {
        cout << "int 的 myFunc" << std::endl;
    }
    void myFunc02(int a, int b = 10)    // 默认参数
    {
        cout << "int, int 的 myFunc" << std::endl;
    }
    void test07()
    {
        // myFunc02(int a) 和 myFunc02(int a, int b = 10)都能识别
        myFunc02(10);   // call of overloaded'myFunc02(int)' is ambiguous
    }
   ```

## 函数重载实现原理

编译器使用不同的参数类型来修饰不同的函数名，比如void func()：编译器可能会将函数名修饰成func，对于void func(int x)，编译器可能将函数名修饰为funcint；对于void func(int x, char c)，编译器会将函数名修改为funcintchar。


# C和C++混合编程

```cpp
extern "c" {    // 这部分代码将会由C语言编译器去编译链接。

}
```

```cpp
// MyModule.h
#ifndef MYMODULE_H
#define MYMODULE_H

#include <stdio.h>

#if __cplusplus
extern "C" {
#endif

    extern void func1();
    extern int func2(int a, int b);

#if _cplusplus
}
#endif

#endif

// MyModule.c
#include "MyModule.h"

void func1()
{
    printf("hello world!\n");
}

int func2(int a, int b)
{
    printf("a + b = %d\n", a + b);
}

// TestExternC.cpp
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include "MyModule.h"
using namespace std;

int main()
{
    func1();
    func2(10, 20);
    return 0;
}
```

> ubuntu:
> main.cpp fun.c fun.h
> 混合编译步骤：
>   gcc -c fun.c -o fun.o
>   g++ main.cpp fun.o -o main


# 四、类和对象

C语言中struct只有变量，C++语言struct既有变量，也有函数。

C语言中，将数据和方法独立，容易造成方法调用错误数据。

```cpp
typedef {
    char name[32];
    int age;
}Person;

typedef {
    int age;
    char name[32];
}Dog;

void PersonEat(void *p) {
    Person *person = (Person *)p;
    printf("%s正在吃饭！\n", person->name);
}

void AnimalEat(void *a) {
    Dog *dog = (Dog *)a;
    printf("%s正在吃狗粮！\n", dog->name);
}

void test()
{
    Person person = {18, "老王"};
    Dog dog = {2. "旺财"};

    PersonEat((void *)&person); // 老王正在吃饭！
    AnimalEat((void *)&dog);    // 旺财正在吃狗粮！
    AnimalEat((void *)&person); // 老王正在吃狗粮！ 用错的情况
}
```

在C语言中，行为和属性是分开的，也就是吃饭这个属性不属于某类对象，而属于所有的共同的数据，所以不单单是PeopleEat可以调用Person数据，AnimalEat也可以调用Person数据，如果调用错误，将会导致问题发生。

因此，属性和行为应该放在一起，一起表示一个具有属性和行为的对象。加入某对象的某项属性不想被外界获知，封装应该再提供一种机制能够给属性和行为的访问权限控制住。

<b>封装特性包含两个方面：一个是属性和变量合成一个整体，一个是给属性和函数增加访问权限。</b>

1. 把变量(属性)和函数(操作)合成一个整体，封装在一个类中
2. 对变量和函数进行访问控制
3. 在类的内部(作用域范围内)，没有访问权限之分，所有成员可以相互访问。
4. 在类的外部(作用域范围外)，访问权限才有意义：public、private、protected.
5. 在类的外部，只有public修饰的成员才能被访问，在没有涉及继承与派生类时，private和protected是同等级的，外部不允许访问。

## 类的初始

```cpp
class 类名 {    // 抽象的概念，系统不会为其分配空间， 但是有大小。
    private:    // 私有 类的外部不可直接访问
    protected:  // 保护；
    数据

    public:     // 公有 类的外部可以直接访问
    方法

    // 在类的内部，没有权限之分，都可以相互访问。
};
```

<b>C++中，struct和class的区别？</b>

> class默认访问权限为private，struct默认访问权限为public。

## 将成员变量设置为private

1. 可赋予客户端访问数据的一致性。如果成员变量不是public，客户端唯一能够访问对象的方法就是通过成员函数。
2. 可细微划分访问控制。使用成员函数可使得我们对变量的控制处理更加精细。如果成员变量访问权限为public，每个人都可以读写它。如果设置为private，可以实现 不准访问、只读访问、读写访问、设置只写访问。

 ## 类的大小

```cpp
class Data
{
private:
    // 成员数据 占类的空间大小
    int num;
public:
    // 成员函数不占用类空间大小，成员函数放在代码区，根据函数名和形参列表去代码区调用函数。
    void setNum(int data)
    {
        num = data;
    }
    int getNum(void)
    {
        return num;
    }
};

void test01()
{
    printf("%d\n", sizeof(Data));   // 4， 4个字节是int num的空间大小
}
```

## 在类里声明，类外定义 成员函数。

> Data.h

```cpp
class Data
{
private:
    int num;
public:
    void setNum(int data);
    int getNum(void);
};
```

> Data.cpp

```cpp
void Data::setNum(int data)
{
    num = data;
}

int Data::getNum(void)
{
    return num;
}
```


## 对象的构造和析构

### 初始化和清理

构造函数和析构函数会被编译器自动调用，完成对象初始化和对象清理工作。

构造函数主要作用于创建对象时为对象的成员属性赋值，构造函数由编译器自动调用，无须手动调用；
析构函数主要用于对象销毁前系统自动调用，执行一些清理工作。

### 构造和析构的定义

<b>构造函数语法</b>：构造函数函数名和类名相同，没有返回值（不能有void），但可以有参数。`class ClassName(){}`

<b>析构函数语法</b>：析构函数函数名在类名前面加“~”，没有返回值（不能有void），不能有参数，不能重载。


### 构造函数的分类

#### 构造函数分类

按照参数类型：无参构造和有参构造 \
按照类型分类：普通构造和拷贝构造函数(复制构造函数)

#### 构造函数调用

无参构造的调用形式：

1. 实例化

```cpp
Data obj1;   // 调用无参或默认构造(隐式调用构造函数)

Data obj2 = Data(); // 显式调用无参构造函数


Data obj3(10);  // 调用有参构造(隐式调用)
Data obj4 = Data(100);  // 显式调用有参构造函数

// 隐式转换的方式，调用有参构造(针对于只有一个数据成员的情况)
Data obj5 = 30;     // 转化成Data obj5(30);

// 匿名对象(当前语句结束，匿名对象立即释放)
Data(40);
```

<b>注意</b>：在同一作用域构造和析构的顺序相反。

#### 拷贝构造函数

```cpp
class Data
{
private:
    int num;
public:
    // 无参构造函数
    Data()
    {
        num = 0;
        cout << "无参构造函数 num = " << num << std::endl;
    }

    // 构造函数（有参的构造）
    Data(int n) 
    {
        num = n;
        cout << "有参构造函数 num = " << num << std::endl;
    }

    // 拷贝构造函数
    Data(const Data &obj)
    {
        num = obj.num;
        cout << "拷贝构造函数 num = " << num << std::endl;
    }

    // 析构函数
    ~Data()
    {
        cout << "析构函数 num = " << num << std::endl;
    }
};

void test03()
{
    Data obj1(10);
    Data obj2 = obj1;   // 调用拷贝构造函数 隐式转换调用
    Data obj3(obj1);    // 调用拷贝构造函数 隐式调用
    Data obj4 = Data(obj1); // 调用拷贝构造函数 显式调用
    // obj4并不会有两次拷贝构造，因为Data(obj1)是拷贝构造，但 obj4 = Data(obj1);是属于对象赋值操作。
}
```

> 调用拷贝构造函数（如果用户不实现拷贝构造，系统将调用默认的拷贝构造） \
> 默认的拷贝构造：单纯的整体赋值（浅拷贝） \
> 如果用户实现了拷贝构造，系统将调用用户实现的拷贝构造

<b>注意</b>：旧对象 初始化新对象，才会调用拷贝构造函数。

```cpp
Data ob1(10);
Data ob2;
ob2 = ob1;  // 不会调用拷贝构造，单纯的对象赋值操作
```

```cpp
Data ob1(); // 不会认为是实例化对象ob1, 而是看成函数ob1声明
```

#### 构造函数的调用规则

C++编译器至少为我们写的类增加3个函数，\
1：默认构造函数(无参，函数体为空)；\
2：默认析构函数(无参，函数体为空)；\
3：默认拷贝构造函数，对类中<b>非静态成员属性简单值拷贝</b>。

如果用户定义了普通构造（非拷贝），C++不再提供默认无参构造，但是会提供默认拷贝构造。

<b>1、如果用户提供了有参构造，将屏蔽系统的默认构造函数。</b> \
```cpp
Data ob1;   // err
```
<b>2、如果用户提供了有参构造，不会屏蔽系统的默认拷贝构造函数。</b> \
```cpp
Data ob1(10);
Data ob2 = ob1; // ob2.num == 10;
```
<b>3、如果用户提供了拷贝构造函数，将屏蔽系统的默认构造函数、默认拷贝构造函数。</b>
```cpp
Data ob1;   // err
```

<b>总结:</b>

对于构造函数，用户一般要实现：无参构造、有参构造、拷贝构造、析构

#### 深拷贝和浅拷贝

同一类型的对象之间可以赋值，使得两个对象的成员变量的值相同，两个对象仍然是独立的两个对象，这种情况被称为浅拷贝。一般情况下，浅拷贝没有任何副作用，但是当类中有指针，并且指针指向动态分配的内存空间，析构函数做了动态内存释放的处理，会导致内存问题。

```cpp
#include <iostream>
#include <cstdlib>
class Person
{
private:
    char m_name[32];
    int m_num;
public:
    Person()
    {
        m_name = NULL;
        m_num = 0;
        cout << "无参构造函数" << std::endl;
    }

    Person(char *name, int num)
    {
        // 为m_name申请空间
        m_name = (char *)calloc(1, strlen(name) + 1);
        if (m_name == NULL) {
            cout << "构造失败" << std::endl;
        }
        strcpy(m_name, name);
        m_num = num;
        cout << "有参构造函数" << std::endl;
    }

    Person(const Person &bob)
    {
        cout << "拷贝构造函数" << std::endl;
        m_name = (char *)calloc(1, strlen(ob.m_name) + 1);
        strcpy(m_name, bob.m_name);
        m_name = bob.m_num;
    }

    ~Person()
    {
        if (m_name != NULL)
        {
            free(m_name);
            m_name = NULL;
        }
        cout << "析构函数" << std::endl;
    }

    void showPerson(void)
    {
        cout << "m_name = " << m_name << ", m_num = " << m_num << std::endl;
    }
};

void test01()
{
    Person lucy("lucy", 18);
    lucy.showPerson();

    // 浅拷贝的问题（多次释放同一块堆区空间）
    // 通过自定义拷贝构造函数，完成深拷贝动作。
    Person bob = lucy;  // 调用系统的默认拷贝构造（单纯的值拷贝）
}
```

#### 初始化列表

初始化列表只能在构造函数中使用。

<b>类的对象作为另一个类的成员</b>

```cpp
class A
{
private:
    int data;
public:
    A()
    {
        cout << "A无参构造" << endl;
    }
    A(int d)
    {
        cout << "A有参构造" << endl;
    }
    ~A()
    {
        cout << "A析构函数" << endl;
    }
};

class B
{
private:
    int data;
public:
    B()
    {
        cout << "A无参构造" << endl;
    }
    B(int d)
    {
        cout << "A有参构造" << endl;
    }
    ~B()
    {
        cout << "A析构函数" << endl;
    }
};

class Data
{
private:
    A oba;  // 对象成员
    B obb;  // 对象成员
    int data;   // 基本类型成员
public:
    Data()
    {
        cout << "Data无参构造" << endl;
    }

    Data(int a, int b, int c)
    {
        cout << "Data有参构造" << endl;
    }

    ~Data()
    {
        cout << "Data析构函数" << endl;
    }
};

void test()
{
    Data data;  // Data的成员对象，A和B都只会调用各自的无参构造函数。由于A和B的成员变量data都是private，所以无法给A和B的对象成员变量data赋值。
}

// 使用初始化列表
Data(int a, int b, int c) : oba(a), obb(b), data(c)
{
    cout << "Data有参构造" << endl;
}
// 使用初始化列表之后，成员变量A oba;和B obb;会被当成声明，然后在初始化列表中调用各自的有参构造函数。
```

先调用对象成员的构造-->自己的构造函数-->析构自己-->析构对象成员

必须在Data的构造函数中，使用初始化列表，使其对象成员显式调用对象成员调用有参构造函数。

<b>注意：</b>

1. 按各对象成员在类定义中的顺序（和参数列表的顺序无关）依次调用它们的构造函数；
2. 先调用对象成员函数的构造函数，再调用本身的构造函数。析构函数和构造函数调用顺序相反，先构造，后析构。

### explict关键字

C++提供了关键字explict，禁止通过构造函数进行的隐式转换。声明为explict的构造函数不能在隐式转换中使用。

<b>注意</b>：explict用于修饰构造函数，防止隐式转换。是针对单参数的构造函数(或者除了第一个参数外其余参数都有默认值的多参构造)而言。其实是前面所描述的隐式转换构造。

### 动态对象创建

创建数组时，总是需要提前预订数组的长度，然后编译器分配预定长度的数组空间；在使用数组时，会存在也许数组空间太大，浪费空间；也许数组空间不足。如果能根据需要来分配空间大小再好不过了。

C早就提供了动态内存分配(dynamic memory allocation)，函数malloc和free可以在运行时从堆中分配存储单元。然而这些函数在C++中不能很好的运行，因为它不能帮我们完成对象的初始化工作。

#### 对象创建

当创建一个C++对象时会发生两件事：
1. 为对象分配内存空间；
2. 调用构造函数来初始化那块内存。

如果使用malloc、calloc、realloc，第一步我们能保证实现；但第二步需要程序员理解和小心使用才能确保第二步一定能发生。

```cpp
Data *p = (Data *)malloc(sizeof(Data));
if (p == NULL)
{
    std::cout << "申请失败" << std::endl;
}

p->Init(100);
p->showNum();

free(p);
```

<b>问题：</b>

1. 程序员必须确定对象的长度；
2. malloc返回一个void指针，C++不允许将void赋值给其他任何指针，必须强转；
3. malloc可能申请内存失败，所以必须判断返回值来确保内存分配成功；
4. 用户在使用对象之前必须记住对它的初始化，<b>构造函数不能显式调用初始化（构造函数是由编译器调用）</b>，用户有可能忘记调用初始化函数。

C的动态分配函数太复杂，容易令人混淆，是不可接受的，C++中我们推荐使用运算符new和delete。

#### new operator

C++中解决动态内存分配的方案是把创建一个对象所需要的操作都结合在一个称为new的运算符里。当用new创建一个对象时，它就在堆里为对象分配内存并调用构造函数完成初始化。

```cpp
Person* person = new Person;
// 相当于：
Person* person = (Person*)malloc(sizeof(Person));
if (person == NULL)
{
    return 0;
}
person->init();
```

1. new给基本类型申请空间

```cpp
void test01()
{
    // 基本类型
    int *p = NULL;
    // p = (int *)calloc(1, sizeof(int));
    p = new int;
    *p = 100;
    cout << "*p = " << *p << endl;

    // 释放 free(p)
    delete p;
}
```

2. new 申请基本类型数组空间

```cpp
void test02()
{
    int *arr = NULL;
    // arr = (int *)calloc(5, sizeof(int));
    arr = new int[5];

    int i = 0;
    for (i = 0; i < 5; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;

    // 释放 new时加了[] delete必须加[]
    delete [] arr;
}
```

> 动态分配数组初始化：`arr = new int[5]{1, 2, 3, 4, 5};`

> delete [] arr;   中括号是一个标志，有[]，编译器会去拿arr的元素个数，然后释放对应个数的对象。

3. new 申请char数组

```cpp
void test04()
{
    // char *arr = new char[32]{"hehe"};   // err, invalid conversion from 'const char*' to 'char'
    // 大括号认为是给某一个元素赋值，只能逐个元素赋值；如果需要整体赋值，需要先申请空间，然后使用strcpy()或者memorycopy进行拷贝。字符串常量是char*
    // 方法一：
    char *arr = new char[32]{'h', 'e', 'h', 'e'};

    // 方法二：
    char *arr = new char[32];
    strcpy(arr, "hehe");
    cout << arr << endl;
    delete [] arr;
}
```

4. new从堆区实例化对象

```cpp
class Person
{
private:
    char m_name[32];
    int  m_num;
public:
    Person()
    {
        cout << "无参构造" << endl;
    }
    Person(char *name, int num)
    {
        strcpy(m_name, name);
        m_num = num;
        cout << "有参构造" << endl;
    }
    ~Person()
    {
        cout << "析构函数" << endl;
    }

    void showPerson()
    {
        cout << "m_name = " << m_name << "， num = " << m_num << endl;
    }
};

void test05()
{
    // new按照Person申请空间，如果申请成功就会自动调用Person类的构造函数。
    Person *person = new Person("lucy", 100);

    // 由于person是指针，使用 -> 如果 person是普通对象，使用`.`
    person->showPerson();
    
    // delete 先调用析构函数，再释放堆区空间
    delete person;
}
```

### 对象数组

本质是数组，只是数组的每个元素是类的对象。










