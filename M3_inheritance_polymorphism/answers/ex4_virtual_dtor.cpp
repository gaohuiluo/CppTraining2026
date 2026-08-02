// 练习 4：虚析构函数（看资源泄漏）
// 编译：cl /EHsc /std:c++17 /W4 ex4_virtual_dtor.cpp
#include <iostream>

class Base {
public:
    Base()  { std::cout << "Base 构造\n"; }

    // 最终版：virtual 析构。
    // 第一步(非虚)现象：若把下面的 virtual 去掉，delete p 只会调 ~Base()，
    //   ~Derived() 被跳过 —— Derived 申请的资源不会释放（泄漏），且是未定义行为。
    virtual ~Base() { std::cout << "Base 析构\n"; }
};

class Derived : public Base {
public:
    Derived()  { std::cout << "Derived 申请资源\n"; }
    ~Derived() override { std::cout << "Derived 释放资源\n"; }
};

int main() {
    Base* p = new Derived;   // 基类指针指向派生类对象
    delete p;                // 虚析构 -> 动态绑定 -> 先 ~Derived 再 ~Base，资源正确释放

    // 结论：只要类可能被当基类、并通过基类指针 delete，
    //       它的析构函数就必须声明为 virtual，否则派生类析构不执行 -> 泄漏 + UB。
    return 0;
}
