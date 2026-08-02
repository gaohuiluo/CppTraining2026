// 编译: cl /EHsc /std:c++17 /W4 ex5_value_category.cpp
// 语法验证: g++ -std=c++17 -Wall -Wextra -fsyntax-only ex5_value_category.cpp
//
// 目标: 用重载区分左值/右值, 理解 T&& 和 std::move 的本质。
#include <iostream>
#include <utility>  // std::move

// 两个重载: 编译器根据实参是左值还是右值自动选
void probe(const int& x) {           // const 左值引用: 绑左值(也能兜底绑右值)
    std::cout << "左值  (" << x << ")\n";
}
void probe(int&& x) {                // 右值引用: 只绑右值, 优先级更高
    std::cout << "右值  (" << x << ")\n";
}

int main() {
    int a = 5;

    probe(a);              // 左值: a 有名字、有地址 -> const int&
    probe(10);             // 右值: 字面量是临时量 -> int&&
    probe(a + 1);          // 右值: a+1 的结果是临时量 -> int&&
    probe(std::move(a));   // 右值: std::move(a) 把左值 a 转成右值引用 -> int&&

    // 回答问题:
    // std::move(a) 命中右值版本, 是因为它把 a 的类型「转换」成了 int&& (右值引用),
    // 于是重载决议选中 probe(int&&)。
    // std::move 本身【没有移动 a】: 它只是个 static_cast, 不产生运行时动作。
    // 这里 a 是 int(没有移动语义), 也压根没被改动; a 依然是 5。
    std::cout << "std::move 之后 a 仍是 " << a << "\n";
    return 0;
}
