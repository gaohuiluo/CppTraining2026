// 练习 10：零依赖单元测试
// 编译：cl /EHsc /std:c++17 /W4 ex10_unittest.cpp
#include <iostream>

static int g_failures = 0;

// 失败时打印表达式文本 + 文件行号，并累加失败计数
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::cerr << "FAIL: " << #cond                                \
                      << " (" << __FILE__ << ":" << __LINE__ << ")\n";    \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

// 被测函数
int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int main() {
    CHECK(clamp(5, 0, 10) == 5);     // 正常值
    CHECK(clamp(-3, 0, 10) == 0);    // 下越界 -> 夹到 lo
    CHECK(clamp(99, 0, 10) == 10);   // 上越界 -> 夹到 hi
    CHECK(clamp(0, 0, 10) == 0);     // 边界
    CHECK(clamp(10, 0, 10) == 10);   // 边界

    // 想体验失败：把下面这行取消注释，会打印 FAIL 且退出码变非零
    // CHECK(clamp(5, 0, 10) == 999);

    if (g_failures == 0) {
        std::cout << "全部通过\n";
        return 0;                    // 成功：退出码 0
    }
    std::cerr << g_failures << " 个失败\n";
    return 1;                        // 失败：非零退出码，CI/CTest 才能识别
}
