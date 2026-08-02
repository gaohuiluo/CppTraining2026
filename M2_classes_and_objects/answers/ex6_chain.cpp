// 练习 6：this 与链式调用
// 编译：cl /EHsc /std:c++17 /W4 ex6_chain.cpp
#include <iostream>
#include <string>

class StringBuilder {
public:
    StringBuilder& append(const std::string& s) {
        result_ += s;
        return *this;              // 返回自身引用 -> 支持链式调用
    }
    const std::string& str() const { return result_; }
private:
    std::string result_;
};

int main() {
    StringBuilder sb;
    sb.append("Hello").append(", ").append("World").append("!");
    std::cout << sb.str() << "\n";   // Hello, World!
    return 0;
}
