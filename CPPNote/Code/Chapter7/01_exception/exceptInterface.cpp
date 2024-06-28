#include <iostream>
#include <string>

void testFunc01()
{
    // 函数内部可以抛出任何异常
    // throw 10;
    // throw 0.0f;
    // throw 'a';
    throw "hehe";
    // std::string str = "qee";
    // throw str;
}

// warning: dynamic exception specifications are deprecated in C++11 [-Wdeprecated]
void testFunc02() throw(int, char, std::string) // error: ISO C++17 does not allow dynamic exception specifications
{
    // throw 10;
    // throw 'a';
    // throw "heeh";   // terminate called after throwing an instance of 'char const*'
    std::string ob = "heihei";
    throw ob;
}

// 函数不抛出任何异常
void testFunc03() throw() // warning: throw will always call terminate() [-Wterminate]
{
    throw 3;    // terminate called after throwing an instance of 'int'
}

void test01()
{
    try {
        testFunc03();
    } catch(int e) {    // 只捕获抛出是int类型的异常
        std::cout << "捕获到int类型异常 e = " << e << std::endl;
    } catch (float e) {// 只捕获抛出是int类型的异常
        std::cout << "捕获到float类型异常 e = " << e << std::endl;
    } catch (char e) {// 只捕获抛出是int类型的异常
        std::cout << "捕获到char类型异常 e = " << e << std::endl;
    } catch (char const *e) {
        std::cout << "捕获到char const *类型异常 e = " << e << std::endl;
    } catch (std::string e) {
        std::cout << "捕获到string异常" << std::endl;
    }
    std::cout << "程序做其他事情" << std::endl;
}

int main(int argc, char **argv)
{
    test01();

    return 0;
}