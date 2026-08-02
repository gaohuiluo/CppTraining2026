// 编译: cl /EHsc /std:c++17 /W4 ex6_move.cpp
// 语法验证: g++ -std=c++17 -Wall -Wextra -fsyntax-only ex6_move.cpp
//
// 目标: 补齐移动构造/移动赋值, 凑齐 Rule of 5, 观察移动后源对象「有效但未指定」。
#include <iostream>
#include <algorithm>

class Buffer {
public:
    explicit Buffer(int n) : size_(n), data_(new int[n]) {
        std::fill(data_, data_ + size_, 0);
        std::cout << "普通构造 @" << static_cast<void*>(data_) << "\n";
    }
    ~Buffer() {
        std::cout << "析构     @" << static_cast<void*>(data_) << "\n";
        delete[] data_;   // data_ 可能是 nullptr(被移动过), delete[] nullptr 安全
    }
    // 拷贝构造: 深拷贝
    Buffer(const Buffer& other) : size_(other.size_), data_(new int[other.size_]) {
        std::copy(other.data_, other.data_ + size_, data_);
        std::cout << "拷贝构造\n";
    }
    // 拷贝赋值
    Buffer& operator=(const Buffer& other) {
        if (this == &other) return *this;
        int* newData = new int[other.size_];
        std::copy(other.data_, other.data_ + other.size_, newData);
        delete[] data_;
        data_ = newData;
        size_ = other.size_;
        std::cout << "拷贝赋值\n";
        return *this;
    }
    // 移动构造: 接管指针, 把源对象置空 (noexcept 很重要, 见 principles 第 11 节)
    Buffer(Buffer&& other) noexcept : size_(other.size_), data_(other.data_) {
        other.data_ = nullptr;   // 源对象置空, 否则它析构时会 double free
        other.size_ = 0;
        std::cout << "移动构造\n";
    }
    // 移动赋值
    Buffer& operator=(Buffer&& other) noexcept {
        if (this == &other) return *this;
        delete[] data_;          // 释放自己的旧资源
        data_ = other.data_;     // 接管源资源
        size_ = other.size_;
        other.data_ = nullptr;   // 掏空源对象
        other.size_ = 0;
        std::cout << "移动赋值\n";
        return *this;
    }
    const int* raw() const { return data_; }
private:
    int  size_;
    int* data_;
};

int main() {
    Buffer a(4);
    Buffer b = a;               // 左值 -> 拷贝构造(深拷贝)
    Buffer c = std::move(a);    // 右值 -> 移动构造(偷家)
    std::cout << "移动后 a.data_ = "
              << static_cast<const void*>(a.raw()) << " (应为 0/nullptr)\n";

    Buffer d(1);
    d = std::move(c);           // 移动赋值

    // 回答: 移动后 a 处于「有效但未指定」状态: data_ 是 nullptr。
    //       它仍能安全析构(delete[] nullptr 无害), 也能重新赋值, 但别依赖它原来的内容。
    return 0;
}
