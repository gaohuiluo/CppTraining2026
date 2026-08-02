// 编译: cl /EHsc /std:c++17 /W4 buffer.cpp
// 语法验证: g++ -std=c++17 -Wall -Wextra -fsyntax-only buffer.cpp
//
// mini 项目: 完整遵守 Rule of 5 的 Buffer(持有堆上 int 数组)。
// 六个函数(普通构造 + 五个特殊成员函数)全部手写, 每个都打印, 用测试观察调用时机。
#include <iostream>
#include <algorithm>
#include <vector>
#include <utility>

class Buffer {
public:
    // 普通构造: 堆上分配 n 个 int 并清零
    explicit Buffer(std::size_t n) : size_(n), data_(new int[n]) {
        std::fill(data_, data_ + size_, 0);
        std::cout << "构造(size=" << size_ << ") @" << static_cast<void*>(data_) << "\n";
    }
    // 析构
    ~Buffer() {
        std::cout << "析构 @" << static_cast<void*>(data_) << "\n";
        delete[] data_;   // data_ 可能已被移动成 nullptr, delete[] nullptr 安全
    }
    // 拷贝构造: 深拷贝
    Buffer(const Buffer& other) : size_(other.size_), data_(new int[other.size_]) {
        std::copy(other.data_, other.data_ + size_, data_);
        std::cout << "拷贝构造 @" << static_cast<void*>(data_) << "\n";
    }
    // 拷贝赋值: 防自赋值 -> 先分配 -> 释放旧 -> 接管
    Buffer& operator=(const Buffer& other) {
        std::cout << "拷贝赋值\n";
        if (this == &other) return *this;
        int* newData = new int[other.size_];
        std::copy(other.data_, other.data_ + other.size_, newData);
        delete[] data_;
        data_ = newData;
        size_ = other.size_;
        return *this;
    }
    // 移动构造: 接管 + 置空源 (noexcept 让 vector 扩容时敢用移动)
    Buffer(Buffer&& other) noexcept : size_(other.size_), data_(other.data_) {
        other.data_ = nullptr;
        other.size_ = 0;
        std::cout << "移动构造 @" << static_cast<void*>(data_) << "\n";
    }
    // 移动赋值: 防自赋值 -> 释放旧 -> 接管 -> 置空源
    Buffer& operator=(Buffer&& other) noexcept {
        std::cout << "移动赋值\n";
        if (this == &other) return *this;
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
        return *this;
    }

    void set(std::size_t i, int v) { data_[i] = v; }
    int  get(std::size_t i) const  { return data_[i]; }
    std::size_t size() const       { return size_; }

    // operator<< 友元, 打印全部元素(回忆 M2)
    friend std::ostream& operator<<(std::ostream& os, const Buffer& b) {
        os << "[";
        for (std::size_t i = 0; i < b.size_; ++i) {
            if (i) os << ", ";
            os << b.data_[i];
        }
        return os << "]";
    }
private:
    std::size_t size_;
    int*        data_;
};

// 按值返回的工厂函数: 观察 RVO
Buffer make(std::size_t n) {
    Buffer local(n);
    for (std::size_t i = 0; i < n; ++i) local.set(i, static_cast<int>(i) * 10);
    return local;   // NRVO: 直接在调用处构造, 通常看不到拷贝/移动
}

int main() {
    std::cout << "--- 1. 普通构造 ---\n";
    Buffer a(4);
    for (std::size_t i = 0; i < 4; ++i) a.set(i, static_cast<int>(i) + 1);
    std::cout << "a = " << a << "\n";

    std::cout << "\n--- 2. 拷贝构造 (Buffer b = a) ---\n";
    Buffer b = a;

    std::cout << "\n--- 3. 拷贝赋值 (c = a) ---\n";
    Buffer c(2);
    c = a;

    std::cout << "\n--- 4. 移动构造 (Buffer d = std::move(a)) ---\n";
    Buffer d = std::move(a);
    std::cout << "移动后 a.size() = " << a.size() << " (a 被掏空)\n";

    std::cout << "\n--- 5. 移动赋值 (e = std::move(b)) ---\n";
    Buffer e(1);
    e = std::move(b);

    std::cout << "\n--- 6. 按值返回 + RVO (Buffer f = make(5)) ---\n";
    Buffer f = make(5);   // 八成只看到 make 内部那一次"构造", 没有多余拷贝/移动
    std::cout << "f = " << f << "\n";

    std::cout << "\n--- 7. push_back 进 vector, 观察扩容用移动(因为 noexcept) ---\n";
    std::vector<Buffer> v;
    v.reserve(2);                      // 先留 2 个位, 避免第一次 push 就扩容
    v.push_back(Buffer(3));            // 临时量 -> 移动构造进容器
    v.push_back(Buffer(3));            // 同上
    std::cout << "  (下一次 push 触发扩容, 旧元素搬迁: 因移动是 noexcept -> 用移动)\n";
    v.push_back(Buffer(3));            // 触发扩容, 已有元素被移动到新内存

    std::cout << "\n--- 结束, 逆序析构 ---\n";
    return 0;

    // ============ 小结(哪种操作调了谁) ============
    // 2 拷贝构造: Buffer b = a;          -> 深拷贝, b 独立一块内存
    // 3 拷贝赋值: c = a;                 -> 释放 c 旧内存, 深拷贝 a
    // 4 移动构造: Buffer d = std::move(a)-> 偷 a 的指针, a 被置空(size=0)
    // 5 移动赋值: e = std::move(b)       -> 释放 e 旧内存, 接管 b 的指针, b 被置空
    // 6 RVO/NRVO: make() 按值返回        -> 编译器直接在 f 处构造 local, 省掉返回时的拷贝/移动,
    //                                       所以你只看到 make 内 1 次"构造", 看不到返回时的搬运。
    // 7 vector 扩容: 元素移动构造是 noexcept -> vector 放心用移动搬迁(否则会退回慢速拷贝)。
}
