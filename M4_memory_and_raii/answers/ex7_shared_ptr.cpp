// 练习 7：shared_ptr 与引用计数
// 编译：cl /EHsc /std:c++17 /W4 ex7_shared_ptr.cpp
#include <iostream>
#include <memory>

class Resource {
public:
    Resource()  { std::cout << "Resource 构造\n"; }
    ~Resource() { std::cout << "Resource 析构\n"; }
};

int main() {
    auto a = std::make_shared<Resource>();          // 引用计数 = 1
    std::cout << "创建后 count = " << a.use_count() << "\n";

    {
        auto b = a;                                 // 拷贝：计数 +1
        std::cout << "拷贝后 count = " << a.use_count() << "\n";   // 2
        // a 和 b 指向同一个 Resource
    }   // b 销毁：计数 -1（回到 1），但还没到 0，不 delete

    std::cout << "内层结束后 count = " << a.use_count() << "\n";    // 1

    std::cout << "-- main 即将结束 --\n";
    return 0;
}   // a 销毁：计数 1->0 -> 此时才真正 delete Resource（打印"Resource 析构"）
    // 结论：只有最后一个 shared_ptr 销毁、计数归零时，资源才被释放。
