// 练习 8：头文件——只放声明
#pragma once                       // 防止本头文件被重复包含
#include <iosfwd>                  // 只需要 ostream 的前置声明，比 <iostream> 轻

class Fraction {
public:
    Fraction(int num, int den);    // 只声明，实现放 .cpp

    Fraction operator+(const Fraction& rhs) const;
    double toDouble() const;

    friend std::ostream& operator<<(std::ostream& os, const Fraction& f);

private:
    int num_;   // 分子
    int den_;   // 分母
};
