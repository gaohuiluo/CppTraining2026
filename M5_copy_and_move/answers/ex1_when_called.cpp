// 编译: cl /EHsc /std:c++17 /W4 ex1_when_called.cpp
// 语法验证: g++ -std=c++17 -Wall -Wextra -fsyntax-only ex1_when_called.cpp
//
// 目标: 分清「初始化 vs 赋值」，看清拷贝构造/拷贝赋值分别在什么时候被调用。
#include <iostream>
#include <string>
#include <utility>  // std::move

class Tracer {
public:
    // 普通构造: 从字符串造一个对象
    explicit Tracer(std::string name) : name_(std::move(name)) {
        std::cout << "[" << name_ << "] 普通构造\n";
    }
    // 拷贝构造: 用一个已存在对象造出新对象 (参数必须是 const 引用)
    Tracer(const Tracer& other) : name_(other.name_) {
        std::cout << "[" << name_ << "] 拷贝构造\n";
    }
    // 拷贝赋值: 把已存在对象的内容赋给另一个已存在对象, 返回 *this 支持链式
    Tracer& operator=(const Tracer& other) {
        name_ = other.name_;
        std::cout << "[" << name_ << "] 拷贝赋值\n";
        return *this;
    }
    ~Tracer() { std::cout << "[" << name_ << "] 析构\n"; }
private:
    std::string name_;
};

// 按值传参: 形参 t 是实参的一个副本 -> 触发拷贝构造
void use(Tracer t) {
    std::cout << "  (进入 use, t 是副本)\n";
    (void)t;
}

int main() {
    Tracer a("A");   // 普通构造
    Tracer b = a;    // 【拷贝构造】! b 正在诞生, 这里的 = 是初始化不是赋值
    Tracer c("C");   // 普通构造
    c = a;           // 【拷贝赋值】! c 早就活着了, 这才是真正的赋值 operator=
    use(a);          // 传参: 形参是 a 的副本 -> 拷贝构造; use 返回时副本析构
    // 作用域结束, c、b、a 逆序析构 (构造的逆序)
    return 0;
}
