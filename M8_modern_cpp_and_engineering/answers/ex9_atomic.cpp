// 练习 9：atomic vs mutex
// 编译：cl /EHsc /std:c++17 /W4 ex9_atomic.cpp
// 本机语法检查：g++ -std=c++17 -Wall -Wextra -pthread -c ex9_atomic.cpp
#include <iostream>
#include <thread>
#include <atomic>

std::atomic<long> counter{0};                  // 原子变量，单次操作不可分割

void addN(int n) {
    for (int i = 0; i < n; ++i)
        ++counter;                             // 原子自增，无需加锁，无数据竞争
}

int main() {
    std::thread t1(addN, 100000);
    std::thread t2(addN, 100000);
    t1.join();
    t2.join();

    std::cout << "counter = " << counter.load() << "\n";  // 期望 200000

    // 为什么这里 atomic 就够：只保护「一个变量的一次自增」。
    // 什么时候必须用 mutex：要保证「多步操作 / 多个变量」的一致性时，
    //   例如 "if (balance >= x) balance -= x;" 这种读后写的复合逻辑，
    //   atomic 保证不了整体原子，必须用 mutex 圈住整段。
    return 0;
}
