// 练习 8：循环引用泄漏 与 weak_ptr 修复
// 编译：cl /EHsc /std:c++17 /W4 ex8_weak_ptr.cpp
#include <iostream>
#include <memory>

// ---------- 第一部分：循环引用导致泄漏 ----------
struct BadNode {
    std::shared_ptr<BadNode> next;   // 双向都用 shared_ptr 互指
    std::shared_ptr<BadNode> prev;
    ~BadNode() { std::cout << "BadNode 析构\n"; }
};

void leak() {
    std::cout << "== leak() ==\n";
    auto a = std::make_shared<BadNode>();   // a 计数 1
    auto b = std::make_shared<BadNode>();   // b 计数 1
    a->next = b;   // b 计数 2
    b->prev = a;   // a 计数 2
}   // 局部 a、b 销毁：各自计数 2->1，但成员还互相拿着对方，谁也到不了 0
    // -> 两个 BadNode 都不析构（看不到"BadNode 析构"）-> 泄漏！

// ---------- 第二部分：用 weak_ptr 打破循环 ----------
struct GoodNode {
    std::shared_ptr<GoodNode> next;   // 一个方向仍用 shared（拥有）
    std::weak_ptr<GoodNode>   prev;   // 另一方向用 weak（不拥有、不加计数）
    ~GoodNode() { std::cout << "GoodNode 析构\n"; }
};

void fixed() {
    std::cout << "== fixed() ==\n";
    auto a = std::make_shared<GoodNode>();
    auto b = std::make_shared<GoodNode>();
    a->next = b;   // b 计数 2
    b->prev = a;   // weak_ptr 不加计数，a 计数仍是 1

    // weak_ptr 用前要 lock() 升级成 shared_ptr（对象可能已销毁）
    if (auto p = b->prev.lock()) {
        std::cout << "通过 weak_ptr 访问到了 prev（对象还活着）\n";
    }
}   // a 计数 1->0 -> 析构；连带 a->next 释放，b 计数 2->1->0 -> 析构。都释放了！

int main() {
    leak();
    std::cout << "（上面若没有析构打印，说明泄漏了）\n\n";
    fixed();
    std::cout << "（上面能看到 GoodNode 析构，说明循环已被打破）\n";
    return 0;
}

// 为什么 weak_ptr 能破环：
//   循环引用的根源是两个对象用 shared_ptr 互相"拥有"，计数互相顶着降不到 0。
//   把其中一个方向改成 weak_ptr，它只"观察"不"拥有"、不增加计数，
//   于是环上至少有一处不阻止对方释放，计数就能正常归零。
