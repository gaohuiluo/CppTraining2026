// 练习 4：静态成员做对象计数
// 编译：cl /EHsc /std:c++17 /W4 ex4_counter.cpp
#include <iostream>

class Widget {
public:
    Widget()  { ++count_; }
    ~Widget() { --count_; }
    static int alive() { return count_; }     // 静态成员函数：无 this

private:
    inline static int count_ = 0;             // C++17：类内直接定义静态成员
};

int main() {
    std::cout << "起始: " << Widget::alive() << "\n";   // 0，用类名访问

    Widget a;
    std::cout << "创建 a 后: " << Widget::alive() << "\n";   // 1
    {
        Widget b, c;
        std::cout << "创建 b,c 后: " << Widget::alive() << "\n";   // 3
    }   // b, c 析构
    std::cout << "内层结束后: " << Widget::alive() << "\n";   // 1
    return 0;
}   // a 析构 -> count_ 回到 0
