// 练习 7：dynamic_cast 与 RTTI
// 编译：cl /EHsc /std:c++17 /W4 ex7_dynamic_cast.cpp
#include <iostream>
#include <string>

class Employee {
public:
    virtual std::string role() const = 0;   // 有虚函数 -> 类型多态 -> 才有 RTTI，dynamic_cast 才能用
    virtual ~Employee() = default;
};

class Manager : public Employee {
public:
    std::string role() const override { return "Manager"; }
    void holdMeeting() const { std::cout << "开会\n"; }
};

class Engineer : public Employee {
public:
    std::string role() const override { return "Engineer"; }
    void writeCode() const { std::cout << "写代码\n"; }
};

void act(Employee* e) {
    // dynamic_cast<T*>：运行时检查真实类型，失败返回 nullptr，所以要判空
    if (Manager* m = dynamic_cast<Manager*>(e)) {
        m->holdMeeting();
    } else if (Engineer* eng = dynamic_cast<Engineer*>(e)) {
        eng->writeCode();
    } else {
        std::cout << "未知角色\n";
    }

    // 对比 static_cast：static_cast<Manager*>(e) 编译期不做检查，
    //   如果 e 其实指向 Engineer，得到一个"看似有效"的错误指针，用它就是 UB。
    // dynamic_cast 要求基类有虚函数：RTTI 信息挂在虚表上，无虚表就没类型信息，编译报错。
}

int main() {
    Manager m;
    Engineer e;
    act(&m);   // 开会
    act(&e);   // 写代码
    return 0;
}
