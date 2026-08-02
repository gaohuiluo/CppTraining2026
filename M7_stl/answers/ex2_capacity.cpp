// 练习 2：观察扩容
// 编译：cl /EHsc /std:c++17 /W4 ex2_capacity.cpp
#include <iostream>
#include <vector>

int main() {
    // 不 reserve：观察 capacity 成倍跳变
    std::vector<int> v;
    std::cout << "不 reserve:\n";
    for (int i = 0; i < 20; ++i) {
        v.push_back(i);
        // size 每次 +1；capacity 只在 size 触顶时才成倍跳一次（如 1,2,4,8,16...）
        std::cout << "  size=" << v.size() << " cap=" << v.capacity() << '\n';
    }

    // 先 reserve(20)：capacity 一步到位，后续 push_back 不再扩容/搬迁
    std::vector<int> w;
    w.reserve(20);                   // 只提升 capacity，size 仍是 0
    std::cout << "reserve(20) 后 cap=" << w.capacity() << " size=" << w.size() << '\n';
    for (int i = 0; i < 20; ++i)
        w.push_back(i);
    std::cout << "填满后 cap=" << w.capacity() << " size=" << w.size() << '\n';

    // 说明：
    // - reserve 后 capacity 不变，是因为空间早已备足，push_back 无需再申请新内存。
    // - 成倍扩容 vs 每次 +1：每次 +1 插入 n 个元素要搬 1+2+...+n≈O(n^2) 次；
    //   成倍增长让总搬运次数是 O(n)，平摊到每次 push_back 是 O(1)。
}
