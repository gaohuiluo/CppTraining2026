// mini 项目：形状面积计算器 —— 实现文件
// 编译：cl /EHsc /std:c++17 /W4 main.cpp shape.cpp
#include "shape.h"

namespace {
    constexpr double kPi = 3.14159265358979323846;   // 本文件内部常量
}

// 类外定义成员函数：返回类型 类名::函数名
double Circle::area() const { return kPi * radius_ * radius_; }
std::string Circle::name() const { return "Circle"; }

double Rectangle::area() const { return width_ * height_; }
std::string Rectangle::name() const { return "Rectangle"; }
