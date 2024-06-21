#include <iostream>
#include <string>

class Html
{
public:
    void header(void)
    {
        std::cout << "网页头部" << std::endl;
    }
    void leftHtml(void)
    {
        std::cout << "左侧栏" << std::endl;
    }
    // 网页主体部分
    void MainBody()
    {
        std::cout << "首页网页主题内容" << std::endl;
    }
    // 网页底部
    void Footer()
    {
        std::cout << "网页底部" << std::endl;
    }
private:
    std::string mTitle;
};

class IOTHtml : public Html
{
public:
    // 网页主体部分
    void MainBody()
    {
        std::cout << "物联网的主页" << std::endl;
    }
};

class Ht : public Html
{
public:
    // 网页主体部分
    void MainBody()
    {
        std::cout << "H5的主页" << std::endl;
    }
};



int main(int argc, char **argv)
{
    IOTHtml ob;
    ob.header();
    ob.leftHtml();
    ob.MainBody();
    ob.Footer();

    std::cout << "===================" << std::endl;

    Ht ob1;
    ob1.header();
    ob1.leftHtml();
    ob1.MainBody();
    ob1.Footer();

    return 0;
}