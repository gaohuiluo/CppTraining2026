// 练习 3：virtual 与多态（对比有无 virtual）
// 编译：cl /EHsc /std:c++17 /W4 ex3_polymorphism.cpp
#include <iostream>
#include <string>

// 第二步的最终版本：name() 是 virtual，多态生效。
// 第一步(非虚)的现象已写在注释里：若去掉下面的 virtual，
//   Shape* p = &c; p->name() 会输出 "Shape"（静态绑定，按指针类型定死）。
class Shape {
public:
    virtual std::string name() const { return "Shape"; }   // 虚函数 -> 多态开关
    virtual ~Shape() = default;                            // 基类析构声明为 virtual
};

class Circle : public Shape {
public:
    std::string name() const override { return "Circle"; }  // override：编译器帮查签名
};

class Square : public Shape {
public:
    std::string name() const override { return "Square"; }
};

int main() {
    Circle c;
    Square s;

    Shape* p = &c;
    // 动态绑定：virtual + 基类指针 -> 运行时看真实类型 -> "Circle"
    std::cout << "p->name() = " << p->name() << "\n";
    p = &s;
    std::cout << "p->name() = " << p->name() << "\n";   // "Square"

    // 对比：
    //   静态绑定(早绑定)：非虚函数，编译期按"指针/引用的静态类型"定死调谁。
    //   动态绑定(晚绑定)：虚函数 + 基类指针/引用，运行期按"对象真实类型"查虚表定谁。
    //   本题若 name() 非虚，上面两行都会输出 "Shape"。
    return 0;
}
