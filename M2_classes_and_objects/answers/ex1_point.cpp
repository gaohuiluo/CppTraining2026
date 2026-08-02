// 练习 1：第一个类
// 编译：cl /EHsc /std:c++17 /W4 ex1_point.cpp
#include <iostream>

class Point {
public:
    Point(int x, int y) : x_(x), y_(y) {}   // 初始化列表，一步到位

    int getX() const { return x_; }         // 只读函数 -> const
    int getY() const { return y_; }

    void print() const {                     // 不改成员 -> const
        std::cout << "(" << x_ << ", " << y_ << ")\n";
    }
private:
    int x_;
    int y_;
};

int main() {
    Point p(3, 4);
    p.print();
    std::cout << "x=" << p.getX() << " y=" << p.getY() << "\n";
    return 0;
}
