// 练习 2：构造/析构顺序
// 编译：cl /EHsc /std:c++17 /W4 ex2_order.cpp
#include <iostream>

class Base {
public:
    Base()  { std::cout << "Base 构造\n"; }
    ~Base() { std::cout << "Base 析构\n"; }
};

class Derived : public Base {
public:
    Derived()  { std::cout << "Derived 构造\n"; }
    ~Derived() { std::cout << "Derived 析构\n"; }
};

int main() {
    std::cout << "--- 创建 Derived ---\n";
    Derived d;   // 构造：先 Base 后 Derived
    std::cout << "--- 离开作用域 ---\n";
    return 0;    // 析构：先 Derived 后 Base（与构造严格相反）

    // 为什么 Base 先构造、Derived 后构造？
    //   派生类对象里包含一个"基类子对象"，必须先把地基(基类部分)建好，
    //   派生类构造函数体才能安全地使用继承来的成员。所以基类先构造。
    // 为什么析构相反(Derived 先)？
    //   析构是构造的逆过程：派生类可能用到基类的资源，得先拆派生类这层，
    //   再拆基类，避免"基类先没了、派生类析构却还想用它"。
}
