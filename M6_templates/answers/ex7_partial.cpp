// 练习 7：类模板偏特化 + 类型识别
// 编译：cl /EHsc /std:c++17 /W4 ex7_partial.cpp
#include <iostream>
#include <string>
#include <cstddef>

// 通用版
template <typename T>
class Inspect {
public:
    static std::string kind() { return "value"; }
};

// 偏特化 1：匹配任意指针类型 T*（模板头非空，带自己的参数 T）
template <typename T>
class Inspect<T*> {
public:
    static std::string kind() { return "pointer"; }
};

// 偏特化 2：匹配任意定长数组 T[N]（带类型参数 T 和非类型参数 N）
template <typename T, std::size_t N>
class Inspect<T[N]> {
public:
    static std::string kind() { return "array"; }
};

int main() {
    std::cout << "int    -> " << Inspect<int>::kind()    << "\n";  // value
    std::cout << "int*   -> " << Inspect<int*>::kind()   << "\n";  // pointer
    std::cout << "double*-> " << Inspect<double*>::kind()<< "\n";  // pointer
    std::cout << "int[4] -> " << Inspect<int[4]>::kind() << "\n";  // array
    return 0;
}
