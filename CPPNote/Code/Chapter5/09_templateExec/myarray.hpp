#ifndef MYARRAY_HPP
#define MYARRAY_HPP

#include <iostream>

template<class T>
class MyArray
{
private:
    T *addr;
    int capacity;
    int size;
public:
    MyArray(int capacity);
    MyArray(const MyArray &ob);
    ~MyArray();

    // 尾插法
    void push_back(const T &val);
    // 遍历数组
    void printArray(void);
};

#endif

template<class T>
MyArray<T>::MyArray(int capacity)
{
    // 对于自定义数据类型，new Person[10]; 数组中的每个元素都会调用无参构造
    this->addr = new T[capacity];
    this->capacity = capacity;
    this->size = 0;
}

template<class T>
MyArray<T>::MyArray(const MyArray &ob)
{
    this->capacity = ob.capacity;
    this->addr = new T[this->capacity];

    int i = 0;
    for (i = 0; i < ob.size; i++)
    {
        this->addr[i] = ob.addr[i];
    }
    this->size = ob.size;
}

template<class T>
MyArray<T>::~MyArray()
{
    if (this->addr != nullptr)
    {
        delete [] this->addr;
        this->addr = nullptr;
    }
}

template<class T>
void MyArray<T>::push_back(const T &val)
{
    // 数组的实际个数size 不超过capacity
    if (this->size == this->capacity) {
        std::cout << "容器已满" << std::endl;
        return;
    }
    this->addr[this->size] = val;
    this->size++;
}

template<class T>
void MyArray<T>::printArray(void)
{
    int i;
    for (i = 0; i < this->size; i++) {
        std::cout << this->addr[i] << " ";
    }
    std::cout << std::endl;
}