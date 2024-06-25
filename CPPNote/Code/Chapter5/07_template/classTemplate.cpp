#include <iostream>
#include <string>

// 类模板
template<class T1, class T2>
class Data
{
    friend void addData(Data<std::string, int> &ob);
public:
    T1 name;
    T2 num;
public:
    Data(T1 name, T2 num)
    {
        this->name = name;
        this->num = num;
        std::cout << "有参构造" << std::endl;
    }
    ~Data()
    {
        std::cout << "析构函数" << std::endl;
    }
    void showPerson()
    {
        std::cout << "name = " << this->name << ", num = " << this->num << std::endl;
    }
};

void addData(Data<std::string, int> &ob)
{
    ob.name += "_vip";
    ob.num += 200;
    return;
}

template<class T1, class T2>
void addData(Data<T1, T2> &ob)
{
    std::cout << "函数模板" << std::endl;
    ob.name += 1000;
    ob.num += "template";
}

void test01()
{
    // 类模板不能进行类型自动推导，c++17开始可以进行类型推导
    // Data ob("德玛西亚", 18);    // error: missing template arguments before ‘ob’ ，必须要指明模板参数类型
    Data<std::string, int> ob1("德玛西亚", 100);
    ob1.showPerson();   // name = 德玛西亚, num = 100

    Data<int, std::string> ob2(200, "提莫");
    ob2.showPerson();   // name = 200, num = 提莫

    Data ob3(100, 200); // 使用C++17编译可以通过
    ob3.showPerson();

    Data ob4("小炮", "德玛"); // 使用C++17编译可以通过
    ob4.showPerson();
}



void test02()
{
    Data<std::string, int> ob("德玛西亚", 19);
    addData(ob);
    ob.showPerson();    // name = 德玛西亚_vip, num = 219

    Data<int, std::string> ob2(19, "晓得");
    addData(ob2);
    ob2.showPerson();
}

int main(int argc, char **argv)
{
    test02();
    return 0;
}