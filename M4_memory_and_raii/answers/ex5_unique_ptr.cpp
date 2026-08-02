// 练习 5：unique_ptr 基础
// 编译：cl /EHsc /std:c++17 /W4 ex5_unique_ptr.cpp
#include <iostream>
#include <memory>
#include <utility>   // std::move

class Resource {
public:
    Resource()  { std::cout << "Resource 构造\n"; }
    ~Resource() { std::cout << "Resource 析构\n"; }
    void use()  { std::cout << "使用 Resource\n"; }
};

int main() {
    auto a = std::make_unique<Resource>();   // 推荐用 make_unique 创建
    a->use();                                 // 像裸指针一样用 ->

    // 不能拷贝：unique_ptr 独占所有权，拷贝会造成两个拥有者
    // auto b = a;   // 编译错误：unique_ptr 的拷贝构造被 = delete 掉了

    // 可以移动：所有权从 a 转给 c，转移后 a 变空
    auto c = std::move(a);
    if (!a) std::cout << "a 现在是空\n";       // a 已被移空
    c->use();

    std::cout << "-- main 即将结束 --\n";
    return 0;
}   // c 离开作用域，自动 delete 底层 Resource（打印"Resource 析构"）
    // 全程没有一句手写 delete
