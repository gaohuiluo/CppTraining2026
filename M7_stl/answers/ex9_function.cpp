// 练习 9：std::function 做回调
// 编译：cl /EHsc /std:c++17 /W4 ex9_function.cpp
#include <iostream>
#include <vector>
#include <functional>

static int negate_val(int x) { return -x; }   // 普通函数

// 回调参数：repeat 不关心 action 具体是什么，只要签名匹配 void(int)
static void repeat(int n, const std::function<void(int)>& action) {
    for (int i = 0; i < n; ++i)
        action(i);
}

int main() {
    std::function<int(int)> f;

    f = [](int x) { return x * x; };     // 装 lambda
    std::cout << "f(5)=" << f(5) << '\n';

    f = negate_val;                       // 同一个变量改装普通函数
    std::cout << "f(5)=" << f(5) << '\n';

    repeat(3, [](int i) { std::cout << "第 " << i << " 次\n"; });

    // 把一堆异构 lambda 存进容器统一保管、依次执行
    std::vector<std::function<void()>> tasks;
    tasks.push_back([] { std::cout << "任务 A\n"; });
    tasks.push_back([] { std::cout << "任务 B\n"; });
    tasks.push_back([] { std::cout << "任务 C\n"; });
    for (const auto& t : tasks) t();

    // 代价：std::function 有类型擦除开销（可能堆分配 + 间接调用），比直接用 lambda 慢。
    // 能用模板参数/auto 直接传 lambda 就别用它；确实要"统一类型来存/传各种可调用对象"才用。
}
