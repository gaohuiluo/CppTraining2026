// 编译: cl /EHsc /std:c++17 /W4 ex2_shallow_crash.cpp
// 语法验证: g++ -std=c++17 -Wall -Wextra -fsyntax-only ex2_shallow_crash.cpp
//
// 目标: 直观看清「编译器默认生成的浅拷贝」对含裸指针的类的破坏。
// 警告: 本题重点是看地址并推理, 真跑到作用域结束会 double free 崩溃 —— 演示用。
#include <iostream>

class Buffer {
public:
    explicit Buffer(int n) : size_(n), data_(new int[n]) {
        std::cout << "构造   @" << static_cast<void*>(data_) << "\n";
    }
    ~Buffer() {
        std::cout << "析构   @" << static_cast<void*>(data_) << " (delete[])\n";
        delete[] data_;
    }
    // 故意什么拷贝函数都不写 -> 编译器生成逐成员拷贝(浅拷贝):
    //   生成的版本相当于  data_ = other.data_;  size_ = other.size_;
    //   注意它只拷贝了指针的「值」(地址), 没有复制指向的那块内存!
    const int* raw() const { return data_; }
private:
    int  size_;
    int* data_;
};

int main() {
    Buffer a(4);
    Buffer b = a;   // 浅拷贝: b.data_ 和 a.data_ 指向同一块堆内存!

    std::cout << "a.data_ = " << static_cast<const void*>(a.raw()) << "\n";
    std::cout << "b.data_ = " << static_cast<const void*>(b.raw()) << "\n";

    // 回答问题:
    // 1. 两个地址【相同】——因为浅拷贝只复制了指针值(地址), 两对象共享同一块内存。
    // 2. 作用域结束时 b 先析构 delete[] 那块内存, 接着 a 析构又 delete[] 同一块。
    // 3. 同一块内存被释放两次 = double free, 未定义行为, 通常直接崩溃。
    // 语法能过是因为编译器认为「拷贝指针」完全合法; 崩溃发生在运行期的二次释放。
    return 0;   // <- 这里之后就是 double free 现场
}
