// 编译: cl /EHsc /std:c++17 /W4 ex7_default_delete.cpp
// 语法验证: g++ -std=c++17 -Wall -Wextra -fsyntax-only ex7_default_delete.cpp
//
// 目标: 用 =default / =delete 控制特殊成员函数, 理解独占语义与 Rule of 0 的联系。
#include <iostream>
#include <string>
#include <memory>   // std::unique_ptr
#include <utility>

// 1. 禁止拷贝的类: 拷贝相关函数 =delete
class NonCopyable {
public:
    NonCopyable() = default;                                // 要默认构造
    NonCopyable(const NonCopyable&) = delete;               // 禁止拷贝构造
    NonCopyable& operator=(const NonCopyable&) = delete;    // 禁止拷贝赋值
};

// 3. 全部 =default: 等价于「一个都不写」, 即 Rule of 0。
//    成员都是「懂事」的类型(int/string), 默认逐成员拷贝/移动就是正确的。
class Trivial {
public:
    Trivial() = default;
    Trivial(const Trivial&) = default;
    Trivial& operator=(const Trivial&) = default;
    Trivial(Trivial&&) noexcept = default;
    Trivial& operator=(Trivial&&) noexcept = default;
    ~Trivial() = default;
    int a = 0, b = 0;
    std::string name;
};

int main() {
    NonCopyable a;
    // NonCopyable b = a;   // 2. 编译错误: 拷贝构造被 =delete, 该类型不允许被拷贝。
    (void)a;

    Trivial t; t.a = 1; t.name = "hi";
    Trivial u = t;              // 默认拷贝: 逐成员复制(string 会自己深拷贝)
    std::cout << u.a << " " << u.name << "\n";

    // 4. 为什么 unique_ptr 拷贝被 =delete、移动允许:
    //    unique_ptr 表达「独占所有权」——一块内存只能有一个 owner。
    //    若允许拷贝, 两个 unique_ptr 指向同一内存, 析构时 double free。
    //    所以标准库把它的拷贝构造/拷贝赋值 =delete, 只留移动来「转移」所有权。
    std::unique_ptr<int> p1(new int(42));
    // std::unique_ptr<int> p2 = p1;          // 编译错误: 拷贝被 delete
    std::unique_ptr<int> p3 = std::move(p1);  // OK: 所有权从 p1 转给 p3, p1 变 nullptr
    std::cout << "*p3=" << *p3 << " p1 空? " << (p1 == nullptr) << "\n";
    return 0;
}
