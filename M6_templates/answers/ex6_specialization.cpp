// 练习 6：函数模板全特化
// 编译：cl /EHsc /std:c++17 /W4 ex6_specialization.cpp
#include <iostream>
#include <string>

// 通用版：兜底
template <typename T>
std::string typeName(T) { return "unknown"; }

// 全特化：template <> 空模板头 + 函数名后写具体类型
template <>
std::string typeName<int>(int) { return "int"; }

template <>
std::string typeName<double>(double) { return "double"; }

template <>
std::string typeName<const char*>(const char*) { return "c-string"; }

int main() {
    std::cout << typeName(42) << "\n";     // int（用特化）
    std::cout << typeName(3.14) << "\n";   // double（用特化）
    std::cout << typeName("hi") << "\n";   // c-string（字面量退化成 const char*）
    std::cout << typeName('x') << "\n";    // unknown（char 没特化，走通用版）
    return 0;
}
