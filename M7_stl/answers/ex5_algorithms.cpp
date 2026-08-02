// 练习 5：accumulate / transform / max_element
// 编译：cl /EHsc /std:c++17 /W4 ex5_algorithms.cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>       // accumulate 在这里，不在 <algorithm>

int main() {
    std::vector<int> v{3, 1, 4, 1, 5, 9, 2, 6};

    int sum = std::accumulate(v.begin(), v.end(), 0);          // 求和，初值 0
    std::cout << "和 = " << sum << '\n';

    // accumulate 带自定义累加器：acc 是累计值，x 是当前元素，求平方和
    int sq = std::accumulate(v.begin(), v.end(), 0,
                             [](int acc, int x) { return acc + x * x; });
    std::cout << "平方和 = " << sq << '\n';

    // transform：把每个元素映射后写回自己（in-place）
    std::vector<int> doubled = v;
    std::transform(doubled.begin(), doubled.end(), doubled.begin(),
                   [](int x) { return x * 2; });
    std::cout << "翻倍: ";
    for (int x : doubled) std::cout << x << ' ';
    std::cout << '\n';

    // max_element / min_element 返回迭代器，解引用得到值
    std::cout << "最大 = " << *std::max_element(v.begin(), v.end())
              << "，最小 = " << *std::min_element(v.begin(), v.end()) << '\n';

    // 陷阱说明：若 v 是 vector<double>，accumulate(...,0) 的初值是 int，
    // 累加过程会被当成 int 运算、把和截断成整数。求 double 的和必须写初值 0.0。
}
