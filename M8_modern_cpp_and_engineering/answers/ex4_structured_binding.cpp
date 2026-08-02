// 练习 4：结构化绑定遍历 map / 拆 pair
// 编译：cl /EHsc /std:c++17 /W4 ex4_structured_binding.cpp
#include <iostream>
#include <map>
#include <string>
#include <utility>

std::pair<bool, int> tryDivide(int a, int b) {
    if (b == 0) return {false, 0};
    return {true, a / b};
}

int main() {
    std::map<std::string, int> scores{{"Alice", 90}, {"Bob", 85}, {"Cara", 78}};

    // const auto& 遍历：不拷贝、只读，遍历 map 首选
    // 对比老写法 it->first / it->second，可读性高一个档次
    for (const auto& [name, score] : scores)
        std::cout << name << ": " << score << "\n";

    // auto& 能改原对象（这里给每人加 5 分）
    for (auto& [name, score] : scores)
        score += 5;
    std::cout << "Alice 加分后: " << scores["Alice"] << "\n";

    // 拆 pair 返回值
    auto [ok, val] = tryDivide(10, 2);
    if (ok) std::cout << "10/2 = " << val << "\n";

    // 说明：auto [..] 是拷贝；auto& 绑引用可改原对象；const auto& 只读不拷贝。
    return 0;
}
