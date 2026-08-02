// 练习 3：构造 + 析构，观察生命周期
// 编译：cl /EHsc /std:c++17 /W4 ex3_lifetime.cpp
#include <iostream>
#include <string>
#include <utility>   // std::move

class Logger {
public:
    Logger(std::string name) : name_(std::move(name)) {
        std::cout << "[" << name_ << "] 构造\n";
    }
    ~Logger() {
        std::cout << "[" << name_ << "] 析构\n";
    }
private:
    std::string name_;
};

int main() {
    Logger a("A");           // a 在 main 作用域，最后才析构
    {
        Logger b("B");       // b 在内层作用域
        std::cout << "-- 内层作用域即将结束 --\n";
    }                        // 这里 b 离开作用域，立刻析构
    std::cout << "-- main 即将结束 --\n";
    return 0;
}   // 这里 a 离开作用域，析构

// 输出顺序：
//   [A] 构造
//   [B] 构造
//   -- 内层作用域即将结束 --
//   [B] 析构        <- b 先销毁：它所在的内层 { } 先结束
//   -- main 即将结束 --
//   [A] 析构
//
// 为什么 B 先于 A 析构：
//   对象在离开其所在作用域时销毁。b 的作用域(内层花括号)比 a 的(整个 main)先结束，
//   所以 b 先析构。同一作用域内的多个对象，则按【构造的逆序】析构(后构造先析构)。
