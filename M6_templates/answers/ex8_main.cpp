// 练习 8：使用 addAll 模板
// 编译：cl /EHsc /std:c++17 /W4 ex8_main.cpp Adder.cpp
//
// 报错版本（请自己先试一次再修）：把 addAll 的定义放进 Adder.cpp、头文件只留声明，
// 编译会得到类似：
//   MSVC:  error LNK2019: unresolved external symbol "int addAll<int>(int const*,int)"
//   g++ :  undefined reference to `int addAll<int>(int const*, int)'
// 原因：编译 ex8_main.cpp 时看不到模板定义，实例化不出 addAll<int>。
// 修复：把定义挪回 Adder.h（本 answers 已是修复版）。
#include <iostream>
#include "Adder.h"

int main() {
    int   ints[]    = {1, 2, 3, 4, 5};
    double doubles[] = {1.5, 2.5, 3.0};

    std::cout << "sum(int)    = " << addAll(ints, 5)    << "\n";   // 15
    std::cout << "sum(double) = " << addAll(doubles, 3) << "\n";   // 7
    return 0;
}
