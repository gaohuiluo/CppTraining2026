// 练习 2：显式指定模板参数 vs 自动推导
// 编译：cl /EHsc /std:c++17 /W4 ex2_explicit_arg.cpp
#include <iostream>

template <typename T>
T half(T x) {
    return x / 2;   // 除法的行为由 T 决定：int 是整除，double 是浮点除
}

int main() {
    // 推导 T=int：10/2 走整数除法 -> 5
    std::cout << "half(10)          = " << half(10) << "\n";

    // 显式指定 T=double：实参 10 先转成 10.0，10.0/2 -> 5（浮点，能出现小数）
    std::cout << "half<double>(10)  = " << half<double>(10) << "\n";

    // 推导 T=double：10.0/2 -> 5
    std::cout << "half(10.0)        = " << half(10.0) << "\n";

    // 结论：half(10) 与 half<double>(10) 结果类型不同，是因为 T 不同——
    // T 决定了用哪种除法。能推导就让编译器推，想强制类型就显式写 <T>。
    return 0;
}
