// 练习 5：constexpr 编译期计算
// 编译：cl /EHsc /std:c++17 /W4 ex5_constexpr.cpp
#include <iostream>

// constexpr 函数：既能编译期算，也能运行期算，看传的实参
constexpr long factorial(int n) {
    long r = 1;
    for (int i = 2; i <= n; ++i) r *= i;   // C++14 起 constexpr 函数里能用循环
    return r;
}

int main() {
    // 编译期求值：f5 是真正的编译期常量，有类型、进符号表、调试器可见
    constexpr long f5 = factorial(5);
    std::cout << "5! = " << f5 << "\n";

    // 能当数组大小（要求编译期常量），宏之外唯有 constexpr 能这么用得安全
    int arr[factorial(4)];                 // 大小 = 24
    std::cout << "arr 大小 = " << sizeof(arr) / sizeof(arr[0]) << "\n";

    // 传运行期变量时，同一个函数退化成普通运行期调用
    int n = 6;
    std::cout << "6! (运行期) = " << factorial(n) << "\n";

    // 对比 #define：宏无类型检查、无作用域、调试器看不到、有重复求值副作用；
    // constexpr 全都没有这些问题，能表达常量就别用宏。
    return 0;
}
