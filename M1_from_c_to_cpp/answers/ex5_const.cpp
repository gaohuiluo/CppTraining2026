// 练习 5：指针 const 三兄弟
// 编译：cl /EHsc /std:c++17 /W4 ex5_const.cpp
#include <iostream>

int main() {
    int x = 1, y = 2;

    // 口诀：从右往左读。

    // const int* p1 —— "p1 是指针，指向 const int"
    //   → 指向的值只读，但指针本身可改指向
    const int* p1 = &x;
    p1 = &y;        // 能：改指向可以
    // *p1 = 10;    // 不能：*p1 只读，不能通过 p1 改值

    // int* const p2 —— "p2 是 const 指针，指向 int"
    //   → 指针本身不可改指向，但指向的值可改
    int* const p2 = &x;
    // p2 = &y;     // 不能：p2 是 const，指向不可改
    *p2 = 10;       // 能：可以改它指向的值

    // const int* const p3 —— "p3 是 const 指针，指向 const int"
    //   → 指向和值都不可改
    const int* const p3 = &x;
    // p3 = &y;     // 不能：指向不可改
    // *p3 = 10;    // 不能：值也只读

    std::cout << "x=" << x << " y=" << y << "\n";  // x 被 *p2 改成了 10
    std::cout << "*p1=" << *p1 << " *p3=" << *p3 << "\n";
    return 0;
}
