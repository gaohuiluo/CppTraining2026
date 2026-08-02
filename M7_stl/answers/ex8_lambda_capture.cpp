// 练习 8：lambda 捕获
// 编译：cl /EHsc /std:c++17 /W4 ex8_lambda_capture.cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> v{3, 7, 1, 9, 4, 6, 2};

    // 值捕获：把 threshold 拷一份进 lambda，只读地用它
    int threshold = 5;
    int n = std::count_if(v.begin(), v.end(),
                          [threshold](int x) { return x > threshold; });
    std::cout << "大于 " << threshold << " 的有 " << n << " 个\n";

    // 引用捕获：直接操作外部的 sum（lambda 里改，外部也变）
    int sum = 0;
    std::for_each(v.begin(), v.end(), [&sum](int x) { sum += x; });
    std::cout << "总和 = " << sum << '\n';

    // mutable：值捕获默认只读，加 mutable 才能改内部那份拷贝
    int count = 0;
    auto counter = [count]() mutable { return ++count; };   // 改的是内部拷贝
    std::cout << "counter: " << counter() << ' ' << counter() << ' ' << counter() << '\n';
    std::cout << "外部 count 仍是 " << count << "（值捕获，互不影响）\n";

    // 场景区分：
    // - 值捕获：需要快照、或 lambda 会存起来晚点用（安全，不怕外部变量销毁）。
    // - 引用捕获：想在 lambda 内改外部变量、或避免拷贝大对象；
    //   风险是 lambda 存活期比被捕获变量长时会悬空（如返回捕获局部变量引用的 lambda）。
}
