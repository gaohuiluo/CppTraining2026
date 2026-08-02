// 练习 2：variant + visit 类型安全联合体
// 编译：cl /EHsc /std:c++17 /W4 ex2_variant.cpp
#include <iostream>
#include <variant>
#include <vector>
#include <string>

using Value = std::variant<int, double, std::string>;

// visitor：每个可能的类型都给一个 operator()，漏一个编译不过
struct Printer {
    void operator()(int i)                { std::cout << "int:    " << i << "\n"; }
    void operator()(double d)             { std::cout << "double: " << d << "\n"; }
    void operator()(const std::string& s) { std::cout << "string: " << s << "\n"; }
};

int main() {
    std::vector<Value> values;
    values.emplace_back(42);
    values.emplace_back(3.14);
    values.emplace_back(std::string("hello"));

    // visit 自动按当前装的类型分派到对应 operator()
    for (const auto& v : values)
        std::visit(Printer{}, v);

    // holds_alternative：先判断当前装的是不是某类型
    Value v = values.front();
    if (std::holds_alternative<int>(v))
        std::cout << "第一个确实是 int\n";

    // get_if：匹配返回指针，不匹配返回 nullptr（不抛异常，适合试探）
    if (auto p = std::get_if<int>(&v))
        std::cout << "get_if 取到: " << *p << "\n";
    if (std::get_if<double>(&v) == nullptr)
        std::cout << "它不是 double\n";
    return 0;
}
