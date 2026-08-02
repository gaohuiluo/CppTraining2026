// 综合 mini：定长整数栈 IntStack
// 编译：cl /EHsc /std:c++17 /W4 stack.cpp
#include <iostream>

class IntStack {
public:
    IntStack() : top_(0) {}

    bool push(int v) {
        if (top_ == CAP) return false;       // 满了
        data_[top_++] = v;
        return true;
    }
    bool pop(int& out) {
        if (top_ == 0) return false;         // 空了
        out = data_[--top_];
        return true;
    }
    bool empty() const { return top_ == 0; }
    int  size()  const { return top_; }

    // 友元非成员，打印从底到顶
    friend std::ostream& operator<<(std::ostream& os, const IntStack& s);

private:
    static constexpr int CAP = 64;           // 容量常量
    int data_[CAP];
    int top_;                                 // 下一个可写位置 = 元素个数
};

std::ostream& operator<<(std::ostream& os, const IntStack& s) {
    os << "[";
    for (int i = 0; i < s.top_; ++i) {
        os << s.data_[i];
        if (i + 1 < s.top_) os << ", ";
    }
    os << "]";
    return os;
}

int main() {
    IntStack st;
    for (int i = 1; i <= 5; ++i) st.push(i * 10);
    std::cout << "压入后: " << st << " size=" << st.size() << "\n";

    int x;
    st.pop(x);  std::cout << "弹出 " << x << "\n";
    st.pop(x);  std::cout << "弹出 " << x << "\n";
    std::cout << "现在: " << st << " size=" << st.size() << "\n";

    // 边界测试：把栈弹空
    while (st.pop(x)) { /* 一直弹 */ }
    std::cout << "弹空后 empty=" << st.empty() << "\n";
    std::cout << "对空栈 pop 返回: " << st.pop(x) << " (0=失败)\n";

    return 0;
}
