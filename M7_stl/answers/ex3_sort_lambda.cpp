// 练习 3：sort + lambda（对比 C 的 qsort）
// 编译：cl /EHsc /std:c++17 /W4 ex3_sort_lambda.cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>       // std::abs

static void print(const std::vector<int>& v, const char* tag) {
    std::cout << tag << ": ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';
}

int main() {
    std::vector<int> v{5, 2, 8, 1, 9, 3};

    std::sort(v.begin(), v.end());                    // 默认升序（用 operator<）
    print(v, "升序");

    // lambda 作为比较器：返回 a 是否应排在 b 前面。a>b 即降序
    std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
    print(v, "降序");

    std::vector<int> u{-5, 2, -8, 1, 9, -3};
    std::sort(u.begin(), u.end(),
              [](int a, int b) { return std::abs(a) < std::abs(b); });  // 按绝对值升序
    print(u, "按绝对值");

    // 对比 C：qsort 需要一个独立的 int cmp(const void* pa, const void* pb) 函数，
    // 函数内还要把 void* 强转回 int* 再解引用；比较逻辑和调用点是割裂的。
    // 这里 lambda 直接写在 sort 调用处，类型安全、无强转、还能被编译器内联，更快。
}
