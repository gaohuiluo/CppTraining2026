// mini 项目：形状面积计算器 —— 头文件（声明）
// 编译：cl /EHsc /std:c++17 /W4 main.cpp shape.cpp
#pragma once
#include <string>

// 抽象基类：定义"形状"这个接口。两个纯虚函数交给派生类实现。
class Shape {
public:
    virtual double area() const = 0;        // 纯虚：Shape 不知道怎么算面积
    virtual std::string name() const = 0;   // 纯虚：形状名字
    virtual ~Shape() = default;             // 虚析构：通过 Shape* 删除派生对象时正确析构
};

class Circle : public Shape {
public:
    explicit Circle(double radius) : radius_(radius) {}
    double area() const override;           // 类外实现放 shape.cpp
    std::string name() const override;
private:
    double radius_;
};

class Rectangle : public Shape {
public:
    Rectangle(double width, double height) : width_(width), height_(height) {}
    double area() const override;
    std::string name() const override;
private:
    double width_;
    double height_;
};
