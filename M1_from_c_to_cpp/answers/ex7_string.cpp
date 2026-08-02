// 练习 7：std::string 练手
// 编译：cl /EHsc /std:c++17 /W4 ex7_string.cpp
#include <iostream>
#include <string>

int main() {
    std::string name;
    std::cout << "输入你的名字: ";
    std::cin >> name;               // >> 读到空白为止(一个单词)

    std::string greeting = "Hello, " + name + "!";   // 直接用 + 拼接
    std::cout << greeting << "\n";

    std::cout << "长度: " << name.size() << "\n";     // 字符数，自动维护
    std::cout << "首字符: " << name.at(0) << "\n";     // at() 会做越界检查

    // 对比 C：这里不用管 char[] 大小、不用 strcpy/strcat、不用担心 '\0'。
    return 0;
}
