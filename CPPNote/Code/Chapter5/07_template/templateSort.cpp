#include <iostream>
#include <string.h>

template<class T>
void myPrintArrayTemplate(T arr[], int len)
{
    for (int i = 0; i < len; i++)
    {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
}

template<class T>
void mySortArrayTemplate(T arr[], int len)
{
    int i = 0, j = 0;
    for (i = 0; i < len - 1; i++) {
        int min = i;
        for (j = min + 1; j < len; j++) {
            if (arr[min] > arr[j]) {
                min = j;
            }
        }
        if (min != i) {
            T tmp = arr[min];
            arr[min] = arr[i];
            arr[i] = tmp;
        }
    }
    return;
}

int main(int argc, char **argv)
{
    char str[] = "hello template";
    int arr[] = {5, 3, 4, 7, 8, 9, 1, 6, 10};
    int str_len = strlen(str);
    int arr_len = sizeof(arr) / sizeof(arr[0]);

    // 用函数模板遍历数组
    myPrintArrayTemplate(str, str_len);
    myPrintArrayTemplate(arr, arr_len);

    // 用函数模板遍历数组
    mySortArrayTemplate(str, str_len);
    mySortArrayTemplate(arr, arr_len);

    myPrintArrayTemplate(str, str_len);
    myPrintArrayTemplate(arr, arr_len);

    return 0;
}