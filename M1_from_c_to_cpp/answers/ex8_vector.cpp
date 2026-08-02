// 练习 8：std::vector 综合
// 编译：cl /EHsc /std:c++17 /W4 ex8_vector.cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> nums;          // 空的动态数组

    std::cout << "输入整数(输入 -1 结束):\n";
    int x;
    while (std::cin >> x) {
        if (x == -1) break;
        nums.push_back(x);          // 追加，容量不够时自动扩容
    }

    std::cout << "元素个数: " << nums.size() << "\n";

    int sum = 0;
    for (const auto& v : nums) sum += v;
    std::cout << "总和: " << sum << "\n";

    if (!nums.empty()) {
        int maxv = nums[0];
        for (const auto& v : nums)
            if (v > maxv) maxv = v;
        std::cout << "最大值: " << maxv << "\n";
    } else {
        std::cout << "没有输入任何数\n";
    }

    // 对比 C：无需 malloc/realloc/free，无需预估容量，无越界风险。
    return 0;
}
