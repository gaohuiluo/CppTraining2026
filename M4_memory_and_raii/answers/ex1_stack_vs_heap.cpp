// 练习 1：栈 vs 堆，new/delete 初体验
// 编译：cl /EHsc /std:c++17 /W4 ex1_stack_vs_heap.cpp
#include <iostream>
#include <string>

class Tracer {
public:
    explicit Tracer(std::string name) : name_(std::move(name)) {
        std::cout << "Tracer(" << name_ << ") 构造\n";
    }
    ~Tracer() {
        std::cout << "Tracer(" << name_ << ") 析构\n";
    }
private:
    std::string name_;
};

int main() {
    Tracer s("stack");                 // 栈对象：随 main 结束自动析构
    Tracer* h = new Tracer("heap");    // 堆对象：new 分配内存并调用构造

    std::cout << "-- main 主体逻辑 --\n";

    delete h;   // 必须手动 delete：调用析构 + 归还内存。忘了就泄漏。

    return 0;
}   // 这里 s 离开作用域，自动析构

// 输出：
//   Tracer(stack) 构造
//   Tracer(heap) 构造
//   -- main 主体逻辑 --
//   Tracer(heap) 析构      <- delete h 触发
//   Tracer(stack) 析构     <- main 结束，栈对象自动析构
//
// 回答：
// - 如果不写 delete h：堆对象的析构【不会】被调用，内存泄漏。
//   堆对象只有 delete 时才析构，程序员不管就没人管。
// - 栈对象 s：【一定】会被自动析构，编译器在离开作用域处插入析构调用，
//   你完全不用操心。这正是 RAII 依赖的机制。
