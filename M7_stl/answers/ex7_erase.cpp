// 练习 7：迭代器失效与 erase-remove
// 编译：cl /EHsc /std:c++17 /W4 ex7_erase.cpp
#include <iostream>
#include <vector>
#include <algorithm>

static void print(const std::vector<int>& v, const char* tag) {
    std::cout << tag << ": ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';
}

int main() {
    // 错误示范（不要真运行）：
    //   for (auto it = v.begin(); it != v.end(); ++it)
    //       if (*it % 2 == 0) v.erase(it);   // erase 后 it 失效，接着 ++it 是未定义行为
    // erase 使被删位置及其后的迭代器失效，循环继续用 it 就悬空了。

    // 正确写法 A：用 erase 的返回值（下一个有效迭代器）接住，不 ++
    std::vector<int> a{1, 2, 3, 4, 5, 6, 7, 8};
    for (auto it = a.begin(); it != a.end(); ) {
        if (*it % 2 == 0) it = a.erase(it);   // 删完接住返回值
        else              ++it;               // 没删才前进
    }
    print(a, "写法A(erase返回值)");

    // 正确写法 B：erase-remove 惯用法
    std::vector<int> b{1, 2, 3, 4, 5, 6, 7, 8};
    // remove_if 把要保留的元素前移、返回新逻辑末尾；erase 再真正删掉末尾那段垃圾
    b.erase(std::remove_if(b.begin(), b.end(), [](int x) { return x % 2 == 0; }),
            b.end());
    print(b, "写法B(erase-remove)");   // 与写法 A 结果一致：1 3 5 7
}
