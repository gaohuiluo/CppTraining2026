// 练习 1：函数模板 vs 宏
// 编译：cl /EHsc /std:c++17 /W4 ex1_mymax.cpp
#include <iostream>

// C 风格的宏：纯文本替换，没有类型概念
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// 函数模板：编译器按类型生成真正的函数
template <typename T>
T myMax(T a, T b) {
    return a > b ? a : b;
}

int main() {
    // 模板推导：实参 int -> T=int；实参 double -> T=double
    std::cout << "myMax(3, 5)      = " << myMax(3, 5) << "\n";
    std::cout << "myMax(1.5, 2.5)  = " << myMax(1.5, 2.5) << "\n";

    // 关键区别 1（副作用）：宏是文本替换，参数可能被求值多次
    int i = 3;
    int r = MAX(i++, 5);     // 展开成 ((i++) > (5) ? (i++) : (5))
    // i++ 走了两次：先在比较里 i 变 4，条件为真又执行右边... 行为诡异
    std::cout << "宏 MAX(i++,5) 后 i = " << i << ", r = " << r << "\n";
    // myMax 是真函数，参数只求值一次，不会有这个坑

    // 关键区别 2（类型检查）：宏无类型检查，模板严格。
    // MAX("ab", "cd") 会"编译通过"却比较指针地址（逻辑错），
    // 而 myMax 若类型不匹配（如 myMax(3, 5.0)）会直接编译报错，逼你写对。

    return 0;
}
