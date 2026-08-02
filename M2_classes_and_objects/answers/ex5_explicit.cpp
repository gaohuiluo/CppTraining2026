// 练习 5：explicit 的作用
// 编译：cl /EHsc /std:c++17 /W4 ex5_explicit.cpp
#include <iostream>

class Buffer {
public:
    // 加了 explicit：禁止 int -> Buffer 的隐式转换
    explicit Buffer(int size) {
        std::cout << "分配 " << size << " 字节\n";
    }
};

void use(const Buffer& b) {
    (void)b;
    std::cout << "使用 buffer\n";
}

int main() {
    // 若构造函数【没有】explicit：
    //   use(100);  能编译 —— 100 被隐式转成 Buffer(100)，
    //   往往不是本意(你可能只是手滑传错了参数)。
    //
    // 加了 explicit 后：
    //   use(100);              // 编译错误：不能把 int 隐式转成 Buffer
    //   use(Buffer(100));      // OK：显式构造，意图清晰

    use(Buffer(100));          // 显式写法，明确表示"我要造一个 Buffer"
    return 0;
}
