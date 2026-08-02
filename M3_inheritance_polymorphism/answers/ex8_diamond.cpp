// 练习 8：菱形继承与 virtual 继承
// 编译：cl /EHsc /std:c++17 /W4 ex8_diamond.cpp
#include <iostream>

class Device {
public:
    int id_ = 0;
    void powerOn() const { std::cout << "设备 " << id_ << " 开机\n"; }
};

// 最终版：Camera、Phone 虚继承 Device，使 Smartphone 中只保留一份 Device。
//
// 第一步(普通继承)现象：若写成 class Camera : public Device / class Phone : public Device，
//   则 Smartphone 里有两份 Device 子对象，s.id_ 有歧义、编译不过，
//   只能写 s.Camera::id_ / s.Phone::id_ 分别访问两份独立副本（逻辑上就错了）。
class Camera : virtual public Device {   // virtual 继承
public:
    void takePhoto() const { std::cout << "拍照\n"; }
};

class Phone : virtual public Device {    // virtual 继承
public:
    void call() const { std::cout << "打电话\n"; }
};

// 菱形顶端 Device 因虚继承只保留一份
class Smartphone : public Camera, public Phone {};

int main() {
    Smartphone s;
    s.id_ = 42;      // 不再歧义：只有一份 Device::id_
    s.powerOn();
    s.takePhoto();
    s.call();

    // 虚继承解决了什么：让菱形顶端的公共基类在最派生类里只存在一份，
    // 消除 id_ 的二义性与数据重复。代价是布局更复杂、有额外的虚基类偏移开销。
    return 0;
}
