// 练习 6：对象切片
// 编译：cl /EHsc /std:c++17 /W4 ex6_slicing.cpp
#include <iostream>

class Animal {
public:
    virtual void speak() const { std::cout << "animal\n"; }
    virtual ~Animal() = default;
};

class Dog : public Animal {
public:
    void speak() const override { std::cout << "woof\n"; }
};

void byValue(Animal a)        { a.speak(); }   // 按值收 -> 一进来就被切片
void byRef(const Animal& a)   { a.speak(); }   // 引用 -> 不切片，保留多态

int main() {
    Dog d;

    Animal& r = d;  r.speak();    // woof  —— 引用，多态生效
    Animal* p = &d; p->speak();   // woof  —— 指针，多态生效
    Animal  a = d;  a.speak();    // animal —— 切片！a 是纯 Animal 对象

    byValue(d);   // animal —— 切片：只拷贝 Animal 那部分，vptr 被设成 Animal 的
    byRef(d);     // woof   —— 引用不拷贝，真实类型仍是 Dog

    // 切片时丢了什么：派生类特有的成员 + 指向 Dog 虚表的 vptr。
    // 结果 a 变成一个货真价实的 Animal，虚表指向 Animal，多态自然失效。
    // 修复：多态一律用基类指针或引用，绝不按值传/存。
    return 0;
}
