// 练习 8：thread + mutex 保护计数器
// 编译：cl /EHsc /std:c++17 /W4 ex8_thread_mutex.cpp
// 本机语法检查：g++ -std=c++17 -Wall -Wextra -pthread -c ex8_thread_mutex.cpp
#include <iostream>
#include <thread>
#include <mutex>

long counter = 0;
std::mutex mtx;

void addN(int n) {
    for (int i = 0; i < n; ++i) {
        std::lock_guard<std::mutex> lk(mtx);   // 构造上锁，析构解锁（异常安全）
        ++counter;                             // 临界区：同一时刻只有一个线程进入
    }
}

int main() {
    std::thread t1(addN, 100000);
    std::thread t2(addN, 100000);
    t1.join();                                 // 等两个线程都结束再读结果
    t2.join();

    // 有锁保护：结果恰好 200000。
    // 若去掉锁，++counter 的「读-改-写」三步会在两线程间交错 ->
    // 发生数据竞争（未定义行为），结果通常小于 200000 且每次不同。
    std::cout << "counter = " << counter << "\n";
    return 0;
}
