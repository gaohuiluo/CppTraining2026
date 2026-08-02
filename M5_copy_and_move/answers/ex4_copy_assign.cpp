// 编译: cl /EHsc /std:c++17 /W4 ex4_copy_assign.cpp
// 语法验证: g++ -std=c++17 -Wall -Wextra -fsyntax-only ex4_copy_assign.cpp
//
// 目标: 实现正确的拷贝赋值运算符, 处理三大陷阱: 自赋值、释放旧资源、异常安全顺序。
#include <iostream>
#include <algorithm>

class Buffer {
public:
    explicit Buffer(int n) : size_(n), data_(new int[n]) {
        std::fill(data_, data_ + size_, 0);
    }
    ~Buffer() { delete[] data_; }

    Buffer(const Buffer& other) : size_(other.size_), data_(new int[other.size_]) {
        std::copy(other.data_, other.data_ + size_, data_);
    }

    // 拷贝赋值运算符: 四步走
    Buffer& operator=(const Buffer& other) {
        if (this == &other) return *this;               // 1. 自赋值检查: a = a 时直接返回
        int* newData = new int[other.size_];            // 2. 先分配新的(万一 new 抛异常, *this 还完好)
        std::copy(other.data_, other.data_ + other.size_, newData);
        delete[] data_;                                 // 3. 分配成功后再释放旧资源
        data_ = newData;                                // 4. 接管新资源
        size_ = other.size_;
        return *this;                                   //    返回 *this 支持链式赋值
    }

    void set(int i, int v) { data_[i] = v; }
    int  get(int i) const  { return data_[i]; }
    int  size() const      { return size_; }
    const int* raw() const { return data_; }
private:
    int  size_;
    int* data_;
};

int main() {
    Buffer a(4);
    for (int i = 0; i < 4; ++i) a.set(i, i + 1);   // a = [1,2,3,4]
    Buffer c(2);
    c.set(0, 100); c.set(1, 200);                   // c = [100,200]

    c = a;   // 拷贝赋值: c 原来那块(size=2)被释放, 重新分配 size=4 并复制 a 的内容
    std::cout << "c 现在 size=" << c.size() << " 首元素=" << c.get(0) << "\n";  // 4, 1

    c = c;   // 自赋值: 有 this==&other 检查, 不崩、内容不丢
    std::cout << "自赋值后 c.get(0)=" << c.get(0) << "\n";  // 仍是 1

    // 地址不同 -> c 和 a 各持一份
    std::cout << "a.data_=" << static_cast<const void*>(a.raw())
              << " c.data_=" << static_cast<const void*>(c.raw()) << "\n";
    return 0;
}
