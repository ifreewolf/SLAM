#include <iostream>

class Animal
{
public:
    Animal()
    {
        data = 100;
    }
    int data;
};

class Horse : public Animal
{
public:
    Horse()
    {
        data = 300;
    }
};

class Donkey : public Animal
{
public:
    Donkey()
    {
        data = 400;
    }
};

class Mule : public Horse, public Donkey
{
};

void test01()
{
    Mule mule;
    // std::cout << mule.data << std::endl;    // error: request for member ‘data’ is ambiguous
    std::cout << mule.Horse::data << std::endl; // 300
    std::cout << mule.Donkey::data << std::endl;    // 400
    // std::cout << mule.Horse::Animal::data << std::endl; // error: ‘Animal’ is an ambiguous base of ‘Mule’
    // std::cout << mule.Donkey::Animal::data << std::endl; // error: ‘Animal’ is an ambiguous base of ‘Mule’
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}