// 编译: cl /EHsc /std:c++17 /W4 ex3_deep_copy.cpp
// 语法验证: g++ -std=c++17 -Wall -Wextra -fsyntax-only ex3_deep_copy.cpp
//
// 目标: 把浅拷贝改成深拷贝, 验证副本各持一份内存、互不影响。
#include <iostream>
#include <algorithm>  // std::copy

class Buffer {
public:
    explicit Buffer(int n) : size_(n), data_(new int[n]) {
        std::fill(data_, data_ + size_, 0);
    }
    ~Buffer() { delete[] data_; }

    // 深拷贝构造: 分配【自己的】内存, 再把内容逐元素复制过来
    Buffer(const Buffer& other) : size_(other.size_), data_(new int[other.size_]) {
        std::copy(other.data_, other.data_ + size_, data_);  // 复制内容而非共享指针
    }

    void set(int i, int v) { data_[i] = v; }
    int  get(int i) const  { return data_[i]; }
    const int* raw() const { return data_; }
private:
    int  size_;
    int* data_;
};

int main() {
    Buffer a(4);
    for (int i = 0; i < 4; ++i) a.set(i, i + 1);  // a = [1,2,3,4]

    Buffer b = a;      // 深拷贝构造: b 拥有一块全新内存, 内容和 a 相同
    b.set(0, 999);     // 只改 b

    std::cout << "a.get(0) = " << a.get(0) << "\n";   // 1  (没被 b 影响)
    std::cout << "b.get(0) = " << b.get(0) << "\n";   // 999

    // 地址不同 -> 证明是两块独立内存
    std::cout << "a.data_ = " << static_cast<const void*>(a.raw()) << "\n";
    std::cout << "b.data_ = " << static_cast<const void*>(b.raw()) << "\n";
    return 0;
}
