// 练习 5：模板函数作用于自定义类型
// 编译：cl /EHsc /std:c++17 /W4 ex5_template_with_class.cpp
#include <iostream>
#include <iomanip>

template <typename T>
T myMax(T a, T b) {
    return a > b ? a : b;   // 这里用到 operator>，实例化时才检查 T 是否支持
}

class Money {
public:
    explicit Money(int cents) : cents_(cents) {}

    // myMax 内部要用 >，所以 Money 必须提供它，否则实例化 myMax<Money> 会报错
    bool operator>(const Money& rhs) const { return cents_ > rhs.cents_; }

    friend std::ostream& operator<<(std::ostream& os, const Money& m);
private:
    int cents_;
};

std::ostream& operator<<(std::ostream& os, const Money& m) {
    os << "$" << m.cents_ / 100 << "."
       << std::setw(2) << std::setfill('0') << m.cents_ % 100;
    return os;
}

int main() {
    Money a{150}, b{299};
    // myMax 能作用于 Money，正是因为 Money 重载了 operator>。
    // 若删掉那个 operator>，这一行会在"实例化 myMax<Money>"时报错。
    std::cout << "较大的是 " << myMax(a, b) << "\n";   // $2.99
    return 0;
}
