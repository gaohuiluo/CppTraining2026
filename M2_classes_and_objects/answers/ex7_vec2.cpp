// 练习 7：运算符重载 Vec2
// 编译：cl /EHsc /std:c++17 /W4 ex7_vec2.cpp
#include <iostream>

class Vec2 {
public:
    Vec2(double x = 0, double y = 0) : x_(x), y_(y) {}

    // 成员形式：a + b -> a.operator+(b)
    Vec2 operator+(const Vec2& rhs) const {
        return Vec2(x_ + rhs.x_, y_ + rhs.y_);
    }
    // 向量乘标量：v * 2.0
    Vec2 operator*(double k) const {
        return Vec2(x_ * k, y_ * k);
    }
    bool operator==(const Vec2& rhs) const {
        return x_ == rhs.x_ && y_ == rhs.y_;
    }

    // 友元非成员：左操作数是 ostream，不能做成员函数
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v);

private:
    double x_, y_;
};

std::ostream& operator<<(std::ostream& os, const Vec2& v) {
    os << "(" << v.x_ << ", " << v.y_ << ")";
    return os;                     // 返回 os -> 支持链式 <<
}

int main() {
    Vec2 a(1, 2), b(3, 4);
    std::cout << "a = " << a << ", b = " << b << "\n";
    std::cout << "a + b = " << (a + b) << "\n";     // (4, 6)
    std::cout << "a * 3 = " << (a * 3.0) << "\n";   // (3, 6)
    std::cout << "a == b ? " << (a == b) << "\n";   // 0 (false)
    return 0;
}
