// 练习 7：异常与 RAII 异常安全
// 编译：cl /EHsc /std:c++17 /W4 ex7_exceptions.cpp
#include <iostream>
#include <stdexcept>

// RAII 类：构造获取、析构释放。异常安全的基石
struct Guard {
    Guard()  { std::cout << "获取资源\n"; }
    ~Guard() { std::cout << "释放资源\n"; }   // 栈展开时也会被调用
};

void risky(bool fail) {
    Guard g;                                   // 局部 RAII 对象
    if (fail)
        throw std::runtime_error("boom");      // 抛出 -> 就地中断，开始栈展开
    std::cout << "正常完成\n";
}

// [[nodiscard]]：调用者忽略返回值时编译器告警
[[nodiscard]] bool checkSomething() { return true; }

int main() {
    try {
        risky(true);
    } catch (const std::exception& e) {        // 按 const 引用捕获，防切片
        std::cout << "捕获异常: " << e.what() << "\n";
    }
    // 关键观察：即使 risky 抛了异常，"释放资源" 也照常打印——
    // 因为栈展开时，已构造的局部对象按逆序自动析构。这就是 RAII + 异常安全。

    checkSomething();   // 忽略 [[nodiscard]] 返回值 -> 触发告警（不影响运行）
    (void)checkSomething();  // 显式丢弃可消除告警
    return 0;
}
