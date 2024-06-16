
```cpp
class Data
{
private:
    int num;
    static int data;
public:
    int getData()
    {
        return data;
    }
    static int getData()
    {
        return data;
    }
};

int Data::data = 100;

void test
{
    Data ob;
    cout << ob.getData() << endl;

    // 1. 静态成员函数属于类，可以通过类名称直接访问
    cout << Data::getDataStatci() << endl;

    // 2. 也可以通过对象名访问(对象共享静态成员函数)
    cout << ob.getDataStatic() << endl;
}
```

<b>注意：</b>
1. 静态成员函数的目的：操作静态成员变量。
2. 静态成员函数不能访问非静态成员变量。
3. 普通成员函数可以访问普通成员变量，也可以访问静态成员变量。
4. 静态成员变量和静态成员函数都有权限之分。

#### const静态成员属性

如果一个类的成员，既要实现共享，又要实现不可改变，那就用const static修饰。定义静态const成员变量时，最好在类内部初始化。

```cpp
class Person
{
public:
    const static int data;
};

const int Person::data = 100;   // 类外定义+初始化，这是可以的，但不允许修改

void test()
{
    // 访问
    cout << Person::data << endl;   // 100
    // 赋值
    // Person::data = 200; //  err, 不能进行修改

}
```

#### 静态案例

1. 静态成员变量统计对象的个数

```cpp
class Data
{
public:
    Data()
    {
        cout << "无参构造" << endl;
        count++;
    }
    Data(const Data &ob)
    {
        cout << "拷贝构造函数" << endl;
        count++;
    }
    ~Data()
    {
        cout << "析构函数" << endl;
        count--;
    }

    static int count;
};

int Data::count = 0;

int main(int argc, char **argv)
{
    Data ob1;
    Data ob2;
    {
        Data ob3;
        Data ob4;
        cout << "对象的个数:" << Data::count << endl;   // 4
    }
    cout << "对象的个数:" << Data::count << endl;   // 2
}
```

2. 单例模式

Singleton(单例)：在单例类的内部实现只生成一个实例，同时它提供一个静态的getInstance()工厂方法，让客户可以访问它的唯一实例。为了防止在外部对其实例化，将其默认构造函数和拷贝构造函数设计为私有；在单例类内部定义了一个Singleton类型的静态对象，作为外部共享的唯一实例。

步骤1： 在单例类内部定义了一个Singleton类型的静态对象，作为外部共享的唯一实例。
步骤2： 提供一个静态的getInstance()工厂方法，让客户可以访问它的唯一实例。
步骤3： 为了防止在外部对其实例化，将其默认构造函数和拷贝构造函数设计为私有。

```cpp
class Singleton
{
private:
    Singleton()
    {

    }
    ~Singleton()
    {

    }
public:
    static Singleton* single = nullptr;
};
```
