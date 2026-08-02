// 练习 1：optional 表达「可能没有值」
// 编译：cl /EHsc /std:c++17 /W4 ex1_optional.cpp
#include <iostream>
#include <optional>
#include <vector>

// 返回类型直接写明「可能没有」——比返回 -1 语义清晰
std::optional<int> findFirstEven(const std::vector<int>& v) {
    for (int x : v)
        if (x % 2 == 0)
            return x;              // 有值：隐式包成 optional
    return std::nullopt;          // 没值：明确返回空
}

int main() {
    std::vector<int> a{1, 3, 4, 7};
    std::vector<int> b{1, 3, 5, 7};

    auto r1 = findFirstEven(a);
    if (r1) {                      // 转 bool：有值为 true
        // 先判断再解引用；对空 optional 用 *r1 是未定义行为（同解空指针）
        std::cout << "a 第一个偶数: " << *r1 << "\n";
    }

    auto r2 = findFirstEven(b);
    // value_or：空时给兜底，省去 if
    std::cout << "b 第一个偶数(没有则 -1): " << r2.value_or(-1) << "\n";

    // value() 在空时会抛 std::bad_optional_access，与 *（UB）不同
    std::cout << "r2 有值吗: " << std::boolalpha << r2.has_value() << "\n";
    return 0;
}
