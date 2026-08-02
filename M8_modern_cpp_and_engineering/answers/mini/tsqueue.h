// mini：线程安全的有界任务队列（头文件即实现，模板类）
// 用 mutex + condition_variable 保护，支持消费者优雅退出。
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <typename T>
class ThreadSafeQueue {
public:
    // 入队：加锁 -> 放数据 -> 唤醒一个等待的消费者
    void push(T item) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            queue_.push(std::move(item));
        }
        cv_.notify_one();   // 唤醒放在解锁后，避免被唤醒者又立刻卡在锁上
    }

    // 阻塞取：队列空就睡；被唤醒且（有数据 或 已关闭）才继续。
    // 返回空 optional 表示「队列已关闭且取空」——消费者据此退出。
    std::optional<T> waitAndPop() {
        std::unique_lock<std::mutex> lk(mtx_);   // cv 必须配 unique_lock
        // 带谓词的 wait：自动处理虚假唤醒，等价于 while(!cond) wait();
        cv_.wait(lk, [this] { return !queue_.empty() || closed_; });
        if (queue_.empty())          // 被唤醒只因为 closed_ 且已空 -> 退出信号
            return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    // 关闭队列：通知所有消费者「不会再有新数据了」，唤醒全部让它们收尾退出
    void close() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            closed_ = true;
        }
        cv_.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return queue_.empty();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return queue_.size();
    }

private:
    mutable std::mutex mtx_;              // mutable：const 成员函数里也能加锁
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool closed_ = false;
};
