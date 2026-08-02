// mini 项目：泛型定长栈 Stack<T, N>
// 编译：cl /EHsc /std:c++17 /W4 stack.cpp
#include <iostream>
#include <string>

// 类型参数 T + 非类型参数 N（默认 64）。定长、栈上、不碰堆。
template <typename T, int N = 64>
class Stack {
public:
    bool push(const T& v) {
        if (top_ == N) return false;      // 满了，拒绝
        data_[top_++] = v;
        return true;
    }
    bool pop(T& out) {
        if (top_ == 0) return false;      // 空了，拒绝
        out = data_[--top_];
        return true;
    }
    const T& top() const { return data_[top_ - 1]; }   // 只看不弹

    bool empty() const    { return top_ == 0; }
    bool full()  const    { return top_ == N; }
    int  size() const     { return top_; }
    int  capacity() const { return N; }

    // 友元模板：operator<< 要访问私有成员，且它自己也是个模板。
    // 用 U/M 另起名，避免和类的 T/N 混淆。
    template <typename U, int M>
    friend std::ostream& operator<<(std::ostream& os, const Stack<U, M>& s);

private:
    T   data_[N];
    int top_ = 0;
};

// 从底到顶打印
template <typename U, int M>
std::ostream& operator<<(std::ostream& os, const Stack<U, M>& s) {
    os << "[";
    for (int i = 0; i < s.top_; ++i) {
        if (i) os << ", ";
        os << s.data_[i];
    }
    os << "]";
    return os;
}

int main() {
    // 1) Stack<int>，默认 N=64
    Stack<int> si;
    si.push(10);
    si.push(20);
    si.push(30);
    std::cout << "si = " << si << " (size " << si.size()
              << "/" << si.capacity() << ")\n";     // [10, 20, 30]

    int out;
    si.pop(out);
    std::cout << "弹出 " << out << " 后 si = " << si << "\n";   // [10, 20]

    // 2) Stack<string, 4>：测满栈边界
    Stack<std::string, 4> ss;
    std::cout << "压入前 4 个: ";
    std::cout << ss.push("a") << ss.push("b")
              << ss.push("c") << ss.push("d") << "\n";   // 1111
    std::cout << "第 5 次 push(满): " << ss.push("e") << "\n";  // 0（false）
    std::cout << "ss = " << ss << " full? " << ss.full() << "\n";

    // 3) 空栈 pop 返回 false
    Stack<int, 2> empty;
    int dummy;
    std::cout << "空栈 pop: " << empty.pop(dummy) << "\n";      // 0（false）
    return 0;
}
