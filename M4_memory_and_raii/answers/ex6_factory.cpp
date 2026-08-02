// 练习 6：unique_ptr 工厂函数 + 虚析构
// 编译：cl /EHsc /std:c++17 /W4 ex6_factory.cpp
#include <iostream>
#include <memory>
#include <string>

// 抽象基类：纯虚 area() + 虚析构
class Shape {
public:
    virtual double area() const = 0;
    virtual ~Shape() { std::cout << "~Shape\n"; }   // 虚析构：通过基类指针删除派生对象时必需
};

class Circle : public Shape {
public:
    explicit Circle(double r) : r_(r) {}
    double area() const override { return 3.14159265 * r_ * r_; }
    ~Circle() override { std::cout << "~Circle\n"; }
private:
    double r_;
};

class Rectangle : public Shape {
public:
    Rectangle(double w, double h) : w_(w), h_(h) {}
    double area() const override { return w_ * h_; }
    ~Rectangle() override { std::cout << "~Rectangle\n"; }
private:
    double w_, h_;
};

// 工厂：返回 unique_ptr<Shape>，所有权转移给调用者
std::unique_ptr<Shape> makeShape(const std::string& kind) {
    if (kind == "circle")    return std::make_unique<Circle>(2.0);
    if (kind == "rectangle") return std::make_unique<Rectangle>(3.0, 4.0);
    return nullptr;
}

int main() {
    auto c = makeShape("circle");
    auto r = makeShape("rectangle");

    if (c) std::cout << "圆面积: " << c->area() << "\n";
    if (r) std::cout << "矩形面积: " << r->area() << "\n";

    return 0;
}   // c、r 离开作用域自动析构。因为有虚析构，通过 Shape 指针也能正确调到
    // ~Circle/~Rectangle（先派生后基类）。若基类析构非 virtual，派生部分不会被析构 -> 泄漏。
