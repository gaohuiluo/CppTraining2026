// 练习 6：enum class 强类型枚举
// 编译：cl /EHsc /std:c++17 /W4 ex6_enum_class.cpp
#include <iostream>
#include <string>

enum class Direction { North, East, South, West };

// 另一个枚举也叫 North，但作用域隔离，不冲突
enum class Status { North, Ok };

std::string toString(Direction d) {
    switch (d) {
        case Direction::North: return "North";
        case Direction::East:  return "East";
        case Direction::South: return "South";
        case Direction::West:  return "West";
    }
    return "?";
}

int main() {
    Direction d = Direction::North;      // 必须带作用域 Direction::
    std::cout << "方向: " << toString(d) << "\n";

    // int x = d;                        // 编译错误：enum class 不隐式转 int
    int code = static_cast<int>(d);      // 要转得显式
    std::cout << "底层值: " << code << "\n";

    // 两个枚举各有 North，互不干扰
    Status s = Status::North;
    std::cout << "比较: " << std::boolalpha
              << (static_cast<int>(s) == static_cast<int>(Direction::North)) << "\n";

    // 好处：不隐式转 int（防误用）、名字有作用域（不污染/不撞名）、可指定底层类型。
    return 0;
}
