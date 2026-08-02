// 练习 8：源文件——放实现
#include "Fraction.h"
#include <iostream>
#include <numeric>   // std::gcd (C++17)
#include <cstdlib>   // std::abs

Fraction::Fraction(int num, int den) : num_(num), den_(den) {
    if (den_ == 0) den_ = 1;                 // 简单防护：分母不为 0
    // 约分：除以最大公约数
    int g = std::gcd(std::abs(num_), std::abs(den_));
    if (g > 1) { num_ /= g; den_ /= g; }
    // 让分母恒为正，符号归到分子
    if (den_ < 0) { num_ = -num_; den_ = -den_; }
}

// a/b + c/d = (a*d + c*b) / (b*d)，构造函数会自动约分
Fraction Fraction::operator+(const Fraction& rhs) const {
    return Fraction(num_ * rhs.den_ + rhs.num_ * den_, den_ * rhs.den_);
}

double Fraction::toDouble() const {
    return static_cast<double>(num_) / den_;
}

std::ostream& operator<<(std::ostream& os, const Fraction& f) {
    os << f.num_ << "/" << f.den_;
    return os;
}
