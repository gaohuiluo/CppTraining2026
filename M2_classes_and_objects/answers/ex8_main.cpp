// 练习 8：主程序
// 编译（多文件一起编）：
//   cl /EHsc /std:c++17 /W4 ex8_main.cpp Fraction.cpp
#include "Fraction.h"
#include <iostream>

int main() {
    Fraction a(1, 2);      // 1/2
    Fraction b(1, 3);      // 1/3
    Fraction c = a + b;    // 5/6

    std::cout << a << " + " << b << " = " << c << "\n";
    std::cout << "= " << c.toDouble() << "\n";

    Fraction d(2, 4);      // 构造时会自动约分成 1/2
    std::cout << "2/4 约分后 = " << d << "\n";
    return 0;
}
