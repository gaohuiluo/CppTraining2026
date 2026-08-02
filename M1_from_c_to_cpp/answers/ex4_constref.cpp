// 练习 4：const 引用传参
// 编译：cl /EHsc /std:c++17 /W4 ex4_constref.cpp
#include <iostream>
#include <string>

// const std::string& : 不拷贝 name(高效)，且函数内不能修改它(安全)
void printInfo(const std::string& name, int age) {
    // name += "x";   // 若取消注释，编译报错，例如(MSVC)：
    //   error C3892: “name”: 不能给常量赋值
    // 原因：name 是 const 引用，编译器禁止任何修改它的操作。
    std::cout << name << " (" << age << "岁)\n";
}

int main() {
    std::string who = "小明";
    int age = 18;
    printInfo(who, age);
    return 0;
}
