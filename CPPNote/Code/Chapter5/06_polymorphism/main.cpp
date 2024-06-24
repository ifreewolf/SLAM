#include <iostream>

class Animal
{
public:
    void sleep(void)
    {
        std::cout << "animal 动态在睡觉" << std::endl;
    }
};

class Cat : public Animal
{
public:
    void sleep(void)
    {
        std::cout << "猫在睡觉" << std::endl;
    }
};

void test01()
{
    Cat cat;
    cat.sleep();    // 猫在睡觉，Animal类的sleep()函数被屏蔽了，编译器就近选择了子类的sleep函数。
    cat.Animal::sleep();    // animal 动态在睡觉，访问的是父类的方法

    // 用基类(指针或引用)保存了子类对象(向上转换)
    Animal* p = new Cat;
    p->sleep(); // animal 动态在睡觉
    Animal animal = Cat();
    animal.sleep(); // animal 动态在睡觉
    Animal &ob = cat;
    ob.sleep(); // animal 动态在睡觉
}

void test02()
{
    // 需求：用基类(指针或引用)保存子类对象，同时还需要操作子类自身成员
    Animal *p = new Cat;
    p->sleep(); // 
}


int main(int argc, char **argv)
{
    // test01();
    test02();
    return 0;
}