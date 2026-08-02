// 挑战题：把 "很 C" 的成绩平均分程序，重写成地道现代 C++
// 编译：cl /EHsc /std:c++17 /W4 ex10_challenge.cpp
#include <iostream>
#include <vector>

// 用 const 引用传入，避免拷贝整个 vector，且函数内不修改它
double average(const std::vector<int>& scores) {
    if (scores.empty()) return 0.0;
    int sum = 0;
    for (const auto& s : scores) sum += s;
    // 关键：转成 double 再除，否则是整数除法
    return static_cast<double>(sum) / scores.size();
}

int main() {
    int n;
    std::cout << "How many scores? ";
    std::cin >> n;

    std::vector<int> scores;          // 不再需要固定大小的 int scores[MAX]
    for (int i = 0; i < n; ++i) {
        int s;
        std::cin >> s;
        scores.push_back(s);          // 动态增长，天然没有 MAX 上限/越界问题
    }

    std::cout << "Average: " << average(scores) << "\n";
    return 0;
}

// 迁移要点回顾：
//   固定数组 int[MAX] + count      -> std::vector<int>(自动管理大小)
//   scanf/printf                   -> std::cin / std::cout
//   (double)sum                    -> static_cast<double>(sum)(C++ 更推荐的转换写法)
//   手动索引循环                    -> 范围 for + const auto&
//   把求平均抽成函数，用 const& 传参 -> 高效且意图清晰
