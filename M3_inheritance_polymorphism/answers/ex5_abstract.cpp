// 练习 5：纯虚函数与抽象类
// 编译：cl /EHsc /std:c++17 /W4 ex5_abstract.cpp
#include <iostream>

// 抽象类：含纯虚函数，不能实例化，充当"接口"
class Drawable {
public:
    virtual void draw() const = 0;          // 纯虚函数：只声明，强制派生类实现
    virtual ~Drawable() = default;          // 抽象基类的析构也要 virtual
};

class Text : public Drawable {
public:
    void draw() const override { std::cout << "绘制文本\n"; }   // 必须实现纯虚函数
};

class Image : public Drawable {
public:
    void draw() const override { std::cout << "绘制图片\n"; }
};

int main() {
    // Drawable d;   // 编译错误：抽象类不能实例化（它有未实现的纯虚函数 draw）

    Text t;
    Image img;
    Drawable* items[] = { &t, &img };   // 基类指针数组，指向不同派生对象

    for (Drawable* d : items) {
        d->draw();   // 多态：运行时分派到各自的 draw()
    }
    return 0;
}
