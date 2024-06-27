#ifndef PERSON_HPP
#define PERSON_HPP

#include <iostream>
#include <string>

template<class T1, class T2>
class Person
{
private:
    T1 name;
    T2 age;
public:
    Person(T1 name, T2 age);
    void showPerson();
};

#endif