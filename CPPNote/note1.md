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
    int(&my_arr)[5] = arr;  // my_arr就是数组arr的别名
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