// 练习 2：printf 改 iostream
// 编译：cl /EHsc /std:c++17 /W4 ex2_convert.cpp
#include <iostream>

int main() {
    int    n     = 5;
    double price = 3.14;
    char   grade = 'A';

    // 对比 C 的 printf("n=%d price=%.2f grade=%c\n", n, price, grade);
    // C++ 里不需要占位符，编译器按类型自动选择如何打印：
    std::cout << "n=" << n
              << " price=" << price
              << " grade=" << grade << "\n";

    // 说明：默认输出的浮点数格式与 %.2f 不同(不会固定两位小数)。
    // 精度/格式控制属于 <iomanip> 的内容(如 std::fixed / std::setprecision)，
    // 这里不展开，M1 阶段能正确打印类型即可。

    return 0;
}
