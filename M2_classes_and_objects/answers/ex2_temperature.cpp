// 练习 2：封装与校验
// 编译：cl /EHsc /std:c++17 /W4 ex2_temperature.cpp
#include <iostream>

class Temperature {
public:
    void setCelsius(double c) {
        if (c < -273.15) c = -273.15;       // 校验：不能低于绝对零度
        celsius_ = c;
    }
    double getCelsius() const { return celsius_; }
    double getFahrenheit() const { return celsius_ * 9.0 / 5.0 + 32.0; }
private:
    double celsius_ = 0.0;                    // 私有，外部无法直接乱设
};

int main() {
    Temperature t;
    t.setCelsius(-300);                       // 非法值，会被夹到 -273.15
    std::cout << "摄氏: " << t.getCelsius() << "\n";
    std::cout << "华氏: " << t.getFahrenheit() << "\n";

    // 封装的价值：外部无法写 t.celsius_ = -300; 绕过校验。
    return 0;
}
