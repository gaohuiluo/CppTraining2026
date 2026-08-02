// mini：零依赖断言测试（单线程验证队列基本正确性）
// 编译（本机语法检查）：g++ -std=c++17 -Wall -Wextra -pthread -c tests.cpp
#include "tsqueue.h"
#include <iostream>

static int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::cerr << "FAIL: " << #cond                                \
                      << " (" << __FILE__ << ":" << __LINE__ << ")\n";    \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

int main() {
    ThreadSafeQueue<int> q;

    // 初始为空
    CHECK(q.empty());
    CHECK(q.size() == 0);

    // 入队后大小/非空正确
    q.push(10);
    q.push(20);
    q.push(30);
    CHECK(!q.empty());
    CHECK(q.size() == 3);

    // FIFO 顺序：先进先出（关闭后单线程取，不会阻塞）
    auto a = q.waitAndPop();
    auto b = q.waitAndPop();
    auto c = q.waitAndPop();
    CHECK(a.has_value() && *a == 10);
    CHECK(b.has_value() && *b == 20);
    CHECK(c.has_value() && *c == 30);
    CHECK(q.empty());

    // 关闭后再取空队列，应得到 nullopt（退出信号），不阻塞
    q.close();
    auto d = q.waitAndPop();
    CHECK(!d.has_value());

    if (g_failures == 0) { std::cout << "全部通过\n"; return 0; }
    std::cerr << g_failures << " 个失败\n";
    return 1;
}
