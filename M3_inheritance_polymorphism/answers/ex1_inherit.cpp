// 练习 1：第一个继承
// 编译：cl /EHsc /std:c++17 /W4 ex1_inherit.cpp
#include <iostream>
#include <string>
#include <utility>

class Animal {
public:
    // 基类构造：用初始化列表初始化 name_
    Animal(std::string name) : name_(std::move(name)) {}

    void introduce() const { std::cout << "我是 " << name_ << "\n"; }

protected:
    std::string name_;   // protected：派生类能直接用，外部不能
};

class Dog : public Animal {          // public 继承：Dog is-a Animal
public:
    // 派生类构造：先在初始化列表里显式调用基类构造，再初始化自己的成员
    Dog(std::string name, std::string breed)
        : Animal(std::move(name)),   // 调基类构造，把 name 交给基类
          breed_(std::move(breed)) {}

    // 直接使用继承来的 protected 成员 name_，无需 base.name
    void bark() const { std::cout << name_ << " 汪汪叫（品种：" << breed_ << "）\n"; }

private:
    std::string breed_;
};

int main() {
    Dog d("Rex", "Husky");
    d.introduce();   // 继承来的基类函数
    d.bark();        // 派生类自己的函数
    return 0;
}
