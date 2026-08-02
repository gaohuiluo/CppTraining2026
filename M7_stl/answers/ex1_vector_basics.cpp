// 练习 1：vector 基本操作
// 编译：cl /EHsc /std:c++17 /W4 ex1_vector_basics.cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v;              // 空 vector，无需预先指定大小（对比 C 的定长数组）
    for (int i = 1; i <= 5; ++i)
        v.push_back(i);              // 尾部追加，容量不够时自动扩容

    // 下标访问：和 C 数组一样，[] 不做越界检查
    std::cout << "下标遍历: ";
    for (std::size_t i = 0; i < v.size(); ++i)
        std::cout << v[i] << ' ';
    std::cout << '\n';

    // 范围 for：底层就是 begin/end 迭代器循环的语法糖
    std::cout << "范围 for: ";
    for (int x : v)
        std::cout << x << ' ';
    std::cout << '\n';

    std::cout << "size = " << v.size() << '\n';

    std::cout << "at(2) = " << v.at(2) << '\n';   // at 越界会抛异常，比 [] 安全

    v.pop_back();                    // 删掉末尾元素，size 减 1（不改 capacity）
    std::cout << "pop_back 后: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << "\nsize = " << v.size() << '\n';
}
