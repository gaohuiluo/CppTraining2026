// 练习 2：new/delete vs malloc/free
// 编译：cl /EHsc /std:c++17 /W4 ex2_new_vs_malloc.cpp
#include <iostream>
#include <cstdlib>   // std::malloc / std::free

class Widget {
public:
    Widget()  { std::cout << "构造\n"; }
    ~Widget() { std::cout << "析构\n"; }
};

int main() {
    std::cout << "== new / delete ==\n";
    Widget* a = new Widget();   // 1) 分配内存 2) 调用构造函数
    delete a;                   // 1) 调用析构函数 2) 归还内存
    // 输出：构造 / 析构 —— 两者都被调用

    std::cout << "== malloc / free ==\n";
    Widget* b = static_cast<Widget*>(std::malloc(sizeof(Widget)));  // 只分配裸内存
    // 注意：这里【没有】"构造"打印——malloc 不调用构造函数，对象没被初始化
    std::free(b);               // 只归还内存，同样【没有】"析构"打印
    // 输出：（空）—— 构造/析构都没被调用

    return 0;
}

// 核心区别总结：
// 1) 构造/析构：new/delete 调用；malloc/free 不调用，只管裸内存。
//    -> 有资源(内部还 new 了东西)的类用 malloc 就是灾难，对象根本没初始化好。
// 2) 类型安全：new 返回正确类型指针，无需强转；malloc 返回 void*，要强转。
// 3) 大小：new 编译器自动算；malloc 要手写 sizeof(T)。
// 4) 失败处理：new 默认抛 std::bad_alloc 异常；malloc 返回 NULL 要检查。
// 铁律：new 配 delete，malloc 配 free，绝不混用。
