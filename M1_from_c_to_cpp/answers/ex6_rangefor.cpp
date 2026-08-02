// 练习 6：范围 for + 修改
// 编译：cl /EHsc /std:c++17 /W4 ex6_rangefor.cpp
#include <iostream>

int main() {
    int arr[6] = {10, 20, 30, 40, 50, 60};

    // 只读遍历：const auto& —— 不拷贝、不可改(推荐的只读写法)
    std::cout << "original: ";
    for (const auto& v : arr) std::cout << v << " ";
    std::cout << "\n";

    // 引用遍历：int& —— 可以就地修改元素
    for (int& v : arr) v += 5;

    std::cout << "after +5: ";
    for (const auto& v : arr) std::cout << v << " ";
    std::cout << "\n";

    // 小提示：如果写 for (int v : arr) v += 5; 改的是拷贝，原数组不变。
    return 0;
}
