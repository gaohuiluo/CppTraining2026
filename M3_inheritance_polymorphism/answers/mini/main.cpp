// mini 项目：形状面积计算器 —— 主程序
// 编译：cl /EHsc /std:c++17 /W4 main.cpp shape.cpp
#include "shape.h"
#include <iostream>
#include <memory>
#include <vector>

int main() {
    // 用 vector<unique_ptr<Shape>> 装不同形状：
    //   为什么不是 vector<Shape>？ Shape 是抽象类，根本无法实例化；
    //   为什么不是 vector<某派生类>？ 那样只能装一种形状，且按值存基类会切片。
    //   存 unique_ptr<Shape>（基类指针）既能多态，又能自动释放（M4 主题）。
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(2.0));
    shapes.push_back(std::make_unique<Rectangle>(3.0, 4.0));
    shapes.push_back(std::make_unique<Circle>(1.5));

    double total = 0.0;
    for (const auto& s : shapes) {
        // 通过基类指针多态调用：运行时分派到 Circle/Rectangle 各自的实现
        std::cout << s->name() << " 面积 = " << s->area() << "\n";
        total += s->area();
    }
    std::cout << "总面积 = " << total << "\n";

    // 思考题回答：
    // 1) 容器为何存 unique_ptr<Shape>？抽象类不能实例化，且按值存基类会对象切片，
    //    丢掉派生类信息与多态。存基类指针(智能指针)才能多态且自动管理生命周期。
    // 2) Shape 析构为何 virtual？这里用 unique_ptr<Shape> 持有派生对象，
    //    容器销毁时通过 Shape* 删除对象，非虚析构会跳过派生类析构 -> 泄漏 + UB。
    return 0;
}
