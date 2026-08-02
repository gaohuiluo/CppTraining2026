// 综合 mini：手写简化版 UniquePtr<T>
// 编译：cl /EHsc /std:c++17 /W4 my_unique_ptr.cpp
#include <iostream>
#include <utility>   // std::exchange / std::move

template <typename T>
class UniquePtr {
public:
    // 构造：接管一个裸指针（默认空）
    explicit UniquePtr(T* p = nullptr) : ptr_(p) {}

    // 析构 = 自动释放（RAII 的核心）
    ~UniquePtr() { delete ptr_; }

    // 禁止拷贝：独占语义，拷贝会导致两个拥有者 -> double free
    UniquePtr(const UniquePtr&)            = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    // 移动构造：把对方的指针"偷"过来，对方置空
    UniquePtr(UniquePtr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)) {}

    // 移动赋值：先释放自己原有资源，再接管对方；防自赋值
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            delete ptr_;                              // 释放自己原来持有的
            ptr_ = std::exchange(other.ptr_, nullptr); // 接管对方并把对方置空
        }
        return *this;
    }

    // 像指针一样用
    T& operator*()  const { return *ptr_; }
    T* operator->() const { return ptr_; }

    T* get() const { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }  // 判空

    // 释放旧的、接管新的
    void reset(T* p = nullptr) {
        delete ptr_;
        ptr_ = p;
    }

    // 放弃所有权，返回裸指针（此后释放责任回到调用者）
    T* release() {
        return std::exchange(ptr_, nullptr);
    }

private:
    T* ptr_;
};

// ---------- 演示用类型 ----------
class Widget {
public:
    explicit Widget(int id) : id_(id) { std::cout << "Widget(" << id_ << ") 构造\n"; }
    ~Widget() { std::cout << "Widget(" << id_ << ") 析构\n"; }
    void hello() const { std::cout << "Widget(" << id_ << ") hello\n"; }
private:
    int id_;
};

int main() {
    UniquePtr<Widget> a(new Widget(1));
    a->hello();               // operator->
    (*a).hello();             // operator*

    // 移动转移所有权：a 交给 b，之后 a 变空
    UniquePtr<Widget> b(std::move(a));
    if (!a) std::cout << "a 现在为空\n";
    b->hello();

    // 移动赋值：c 先拿 2 号，再被 b(1号) 覆盖 -> 2号在赋值时被释放
    UniquePtr<Widget> c(new Widget(2));
    c = std::move(b);
    c->hello();

    std::cout << "-- main 即将结束 --\n";
    return 0;
}   // c 离开作用域自动 delete（1号）。全程无手写 delete，独占语义 + 移动都靠自己实现的类完成。
