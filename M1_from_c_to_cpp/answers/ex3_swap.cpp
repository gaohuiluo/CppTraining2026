// 练习 3：引用做 swap
// 编译：cl /EHsc /std:c++17 /W4 ex3_swap.cpp
#include <iostream>

// C 风格：传指针，函数体内要解引用 *
void swap_ptr(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

// C++ 风格：传引用，函数体内像普通变量一样用
void swap_ref(int& a, int& b) {
    int t = a;
    a = b;
    b = t;
}

int main() {
    int x = 1, y = 2;
    std::cout << "before: x=" << x << " y=" << y << "\n";
    swap_ptr(&x, &y);            // 调用处要取地址 &
    std::cout << "after swap_ptr: x=" << x << " y=" << y << "\n";

    int p = 10, q = 20;
    std::cout << "before: p=" << p << " q=" << q << "\n";
    swap_ref(p, q);              // 调用处直接传变量，更干净
    std::cout << "after swap_ref: p=" << p << " q=" << q << "\n";

    // 差异小结：
    //   指针版：声明用 *，调用用 &，函数体用 *，语法负担重，且指针可能为空。
    //   引用版：调用像传普通变量，函数体像用普通变量，且引用一定绑定到有效对象。
    return 0;
}
