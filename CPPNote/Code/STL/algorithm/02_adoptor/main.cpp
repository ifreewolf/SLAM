#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

void myPrint(int val, int tmp)
{
    cout << val + tmp << " ";
}

// 适配器2：公共继承binary_function
// 适配器3：参数的萃取，<int, int, void>
// 适配器4：对operator()进行const修饰
class MyPrint : public binary_function<int, int, void>
{
public:
    void operator()(int val, int tmp) const
    {
        cout << "val = " << val << ", tmp = " << tmp << ", sum = " << val + tmp << endl;
    }
};

void test01()
{
    vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    v.push_back(40);
    v.push_back(50);

    // for_each(v.begin(), v.end(), myPrint); // error: too few arguments to function
    // 出错的原因：需求是打印的时候加上一个动态传入的值，但是for_each读取的v中每个元素都只有一个参数，而myPrint希望传入两个参数。
    // 修改:使用函数适配器，扩展函数参数
    // for_each(v.begin(), v.end(), bind2nd(myPrint, 10)); // error: ‘void(int, int)’ is not a class, struct, or union type
    // bind2nd只支持类、结构体或者union类型

    // 适配器1：bind2nd或bind2st绑定参数
    for_each(v.begin(), v.end(), bind2nd(MyPrint(), 20)); // error: no type named ‘first_argument_type’ in ‘class MyPrint’
    // 这个类必须继承binary_function
    // bind2nd将参数绑定到第二个参数位置
    cout << endl;
    for_each(v.begin(), v.end(), bind1st(MyPrint(), 40));
    // bind1st将参数绑定到第一个参数位置
}

/*
val = 10, tmp = 20, sum = 30
val = 20, tmp = 20, sum = 40
val = 30, tmp = 20, sum = 50
val = 40, tmp = 20, sum = 60
val = 50, tmp = 20, sum = 70

val = 40, tmp = 10, sum = 50
val = 40, tmp = 20, sum = 60
val = 40, tmp = 30, sum = 70
val = 40, tmp = 40, sum = 80
val = 40, tmp = 50, sum = 90
*/
class MyGreaterThan3 : public unary_function<int, bool>
{
public:
    // 一元谓词
    bool operator()(int val) const
    {
        return val > 3;
    }
};
void test02()
{
    vector<int> v;
    for (int i = 0; i < 10; i++)
    {
        v.push_back(i);
    }

    // 找出第一个大于3的数
    vector<int>::iterator it;
    it = find_if(v.begin(), v.end(), MyGreaterThan3());
    if (it != v.end()) {
        cout << "*it = " << *it << endl; // 4
    }

    // 找出第一个小于等于3的数
    it = find_if(v.begin(), v.end(), not1(MyGreaterThan3())); // error: no type named ‘argument_type’ in ‘class MyGreaterThan3’
    if (it != v.end()) {
        cout << "*it = " << *it << endl; // 0
    }
}

class MyGreaterInt : public binary_function<int, int, bool>
{
public:
    bool operator()(int v1, int v2) const
    {
        return v1 > v2;
    }
};
void test03()
{
    vector<int> v;
    v.push_back(2);
    v.push_back(5);
    v.push_back(1);
    v.push_back(4);
    v.push_back(3);

    for_each(v.begin(), v.end(), [](int v) { cout << v << " ";}); // 2 5 1 4 3
    cout << endl;

    // 默认从小到大
    sort(v.begin(), v.end()); // 1 2 3 4 5
    // 更改排序规则，从大到小
    sort(v.begin(), v.end(), MyGreaterInt()); // 5 4 3 2 1
    // 使用not2对MyGreaterInt()取反，从小到大
    sort(v.begin(), v.end(), not2(MyGreaterInt())); // 1 2 3 4 5
    // 使用not2对内联函数取反
    sort(v.begin(), v.end(), not2(greater<int>())); // 1 2 3 4 5
    // 也可以直接使用内联函数实现从小到大排序
    sort(v.begin(), v.end(), less<int>()); // 1 2 3 4 5
    for_each(v.begin(), v.end(), [](int v) { cout << v << " ";});
    cout << endl;
}

class Person
{
public:
    string name;
    int age;
    Person(string name, int age)
    {
        this->name = name;
        this->age = age;
    }
    void showPerson()
    {
        cout << "name = " << this->name << ", age = " << this->age << endl;
    }
};

void myPrintPerson(Person &ob)
{
    cout << "name = " << ob.name << ", age = " << ob.age << endl;
}

void test04()
{
    vector<Person> v;
    v.push_back(Person("德玛西亚", 18));
    v.push_back(Person("狗头", 28));
    v.push_back(Person("牛头", 19));
    v.push_back(Person("小法", 38));

    // 遍历 myPrintPerson普通函数
    // for_each(v.begin(), v.end(), myPrintPerson);

    // 遍历 Person成员函数
    // 利用mem_fun_ref将Person内部成员函数转成适配器
    for_each(v.begin(), v.end(), mem_fun_ref(&Person::showPerson));
}

// 函数指针适配器 ptr_fun
void test05()
{
    vector<int> v;
    for (int i = 0; i < 10; i++)
    {
        v.push_back(i);
    }
    // ptr_fun()把一个普通的函数指针适配成函数对象
    for_each(v.begin(), v.end(), bind2nd(ptr_fun(myPrint), 100)); // 100 101 102 103 104 105 106 107 108 109
    cout << endl;
}

int main(int argc, char **argv)
{
    test05();

    return 0;
}