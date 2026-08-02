// 练习 3：双类型参数的类模板 MyPair
// 编译：cl /EHsc /std:c++17 /W4 ex3_pair.cpp
#include <iostream>
#include <string>

// 两个类型参数：A、B 各自独立
template <typename A, typename B>
class MyPair {
public:
    A first;
    B second;

    // 用初始化列表初始化两个成员
    MyPair(const A& a, const B& b) : first(a), second(b) {}

    void print() const {
        std::cout << "(" << first << ", " << second << ")\n";
    }
};

int main() {
    // 实例化时必须写明两个类型（类模板不自动推导，入门先手写清楚）
    MyPair<int, std::string> p1(1, "one");
    MyPair<double, char>     p2(3.14, 'x');

    p1.print();   // (1, one)
    p2.print();   // (3.14, x)

    // MyPair<int,string> 和 MyPair<double,char> 是两个完全不同的类型
    return 0;
}
