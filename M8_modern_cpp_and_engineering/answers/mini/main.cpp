// mini：生产者-消费者 demo
// 1 个生产者 push 0..99，2 个消费者并发处理，用 atomic 累加统计。
// 编译（本机语法检查）：g++ -std=c++17 -Wall -Wextra -pthread -c main.cpp
// 正式构建见 CMakeLists.txt。
#include "tsqueue.h"
#include <thread>
#include <atomic>
#include <vector>
#include <iostream>

int main() {
    ThreadSafeQueue<int> q;
    std::atomic<long> processedCount{0};   // 处理了多少个任务
    std::atomic<long> sum{0};              // 所有任务值之和

    const int kTasks = 100;

    // 生产者：push 0..99，然后 close 通知消费者收工
    std::thread producer([&] {
        for (int i = 0; i < kTasks; ++i)
            q.push(i);
        q.close();
    });

    // 2 个消费者：不停 waitAndPop，取到 nullopt（队列关闭且空）就退出
    auto consume = [&] {
        while (auto item = q.waitAndPop()) {
            sum += *item;
            ++processedCount;
        }
    };
    std::thread c1(consume);
    std::thread c2(consume);

    producer.join();       // 必须 join，否则线程对象析构会 terminate
    c1.join();
    c2.join();

    // 期望：100 个任务，和 = 0+1+...+99 = 4950
    std::cout << "处理任务数: " << processedCount.load() << "\n";
    std::cout << "总和:       " << sum.load() << "\n";

    long expectedSum = static_cast<long>(kTasks) * (kTasks - 1) / 2;
    if (processedCount.load() == kTasks && sum.load() == expectedSum) {
        std::cout << "结果正确\n";
        return 0;
    }
    std::cout << "结果错误\n";
    return 1;
}
