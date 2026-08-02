// 编译: cl /EHsc /std:c++17 /W4 ex8_rule_of_zero.cpp
// 语法验证: g++ -std=c++17 -Wall -Wextra -fsyntax-only ex8_rule_of_zero.cpp
//
// 目标: Rule of 0 —— 把资源交给 std::vector, 一个特殊成员函数都不写。
#include <iostream>
#include <vector>
#include <utility>

// 对比 ex6 手写 Rule of 5(六个函数): 这里【零个】特殊成员函数。
// 因为唯一的数据成员 std::vector 自己就正确实现了 Rule of 5:
//   - 默认拷贝  -> 调 vector 的拷贝(深拷贝)
//   - 默认移动  -> 调 vector 的移动(偷家)
//   - 默认析构  -> 调 vector 的析构(释放)
// 编译器生成的默认版本会逐成员调用它们, 自动正确、自动安全。
class Buffer {
public:
    explicit Buffer(int n) : data_(static_cast<std::size_t>(n), 0) {}
    // 不写析构、不写拷贝、不写移动 —— 全靠 vector 代劳。

    void set(int i, int v) { data_[static_cast<std::size_t>(i)] = v; }
    int  get(int i) const  { return data_[static_cast<std::size_t>(i)]; }
    std::size_t size() const { return data_.size(); }
    const int* raw() const { return data_.data(); }
private:
    std::vector<int> data_;   // 让 vector 去操心 Rule of 5
};

int main() {
    Buffer a(4);
    for (int i = 0; i < 4; ++i) a.set(i, i + 1);

    Buffer b = a;             // 默认拷贝 = vector 深拷贝
    b.set(0, 999);
    std::cout << "a.get(0)=" << a.get(0) << " b.get(0)=" << b.get(0) << "\n";  // 1 999
    std::cout << "地址不同? a=" << static_cast<const void*>(a.raw())
              << " b=" << static_cast<const void*>(b.raw()) << "\n";

    Buffer c = std::move(a);  // 默认移动 = vector 移动(a 内部 vector 被掏空)
    std::cout << "c.size()=" << c.size() << " 移动后 a.size()=" << a.size() << "\n";

    // 小结: 比 ex6 少写了 6 个函数, 依然安全 —— 因为深拷贝/移动/释放全由 vector 负责。
    // 这就是现代 C++ 的默认姿势: 能 Rule of 0 就 Rule of 0。
    return 0;
}
