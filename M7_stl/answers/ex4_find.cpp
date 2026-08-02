// 练习 4：find / find_if / count_if
// 编译：cl /EHsc /std:c++17 /W4 ex4_find.cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> v{4, 7, 2, 9, 4, 1, 4};

    // find：找具体的值，返回迭代器；没找到返回 end()
    if (auto it = std::find(v.begin(), v.end(), 9); it != v.end())
        std::cout << "找到 9，下标 " << (it - v.begin()) << '\n';   // 迭代器相减得偏移
    else
        std::cout << "没有 9\n";

    // find_if：找第一个满足谓词的元素
    if (auto it = std::find_if(v.begin(), v.end(),
                               [](int x) { return x > 5; }); it != v.end())
        std::cout << "第一个 >5 的是 " << *it << '\n';

    // count：统计某个值出现次数
    std::cout << "4 出现了 " << std::count(v.begin(), v.end(), 4) << " 次\n";

    // count_if：统计满足谓词的元素个数
    std::cout << "偶数有 "
              << std::count_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; })
              << " 个\n";
}
