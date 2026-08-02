// 练习 3：string_view 与悬垂
// 编译：cl /EHsc /std:c++17 /W4 ex3_string_view.cpp
#include <iostream>
#include <string>
#include <string_view>

// 参数用 string_view：只读、不拷贝，能接 string / 字面量 / 另一个 view
bool startsWith(std::string_view s, std::string_view prefix) {
    if (prefix.size() > s.size()) return false;
    // substr 返回的还是视图，不拷贝底层字符
    return s.substr(0, prefix.size()) == prefix;
}

// 错误示范（仅注释，不真的调用）：
//   std::string_view bad() {
//       std::string local = "temp";
//       return local;           // 返回后 local 析构，view 悬垂 -> UB
//   }
// 正确做法：要返回/持有字符串就返回 std::string（拥有数据），别返回 view。

int main() {
    std::string str = "hello world";
    std::cout << std::boolalpha;
    std::cout << startsWith(str, "hello") << "\n";  // 从 string 来，不拷贝
    std::cout << startsWith("abcdef", "abc") << "\n"; // 从字面量来，不构造 string
    std::cout << startsWith("abc", "abcdef") << "\n"; // prefix 更长 -> false

    // string_view 不保证 \0 结尾，不能直接当 C 字符串；要用先转 std::string
    std::string_view sv = str;
    std::string owned{sv};   // 需要 C 字符串 / 需要持有时，转成 string
    std::cout << "c_str: " << owned.c_str() << "\n";
    return 0;
}
