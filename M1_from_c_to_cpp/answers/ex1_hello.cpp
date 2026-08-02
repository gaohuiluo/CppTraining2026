// 练习 1：Hello, iostream
// 编译：cl /EHsc /std:c++17 /W4 ex1_hello.cpp
#include <iostream>

int main() {
    std::cout << "Hello, C++!" << "\n";

    int year = 2026;
    std::cout << "Year: " << year << "\n";

    // 关于 std::endl 与 "\n" 的区别：
    //   "\n"        只输出换行符。
    //   std::endl   输出换行符，并额外【刷新缓冲区】(把缓冲的内容立刻写到终端)。
    // 循环里大量输出时，频繁 std::endl 会因反复刷新而变慢；
    // 平时换行用 "\n" 即可，需要立即看到输出(如调试)时才用 std::endl。
    std::cout << "done" << std::endl;

    return 0;
}
