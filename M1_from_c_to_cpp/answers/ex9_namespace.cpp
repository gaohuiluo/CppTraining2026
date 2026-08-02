// 练习 9：命名空间 + 综合
// 编译：cl /EHsc /std:c++17 /W4 ex9_namespace.cpp
#include <iostream>

namespace metric {
    void describe() { std::cout << "Using meters and kilograms\n"; }
}

namespace imperial {
    void describe() { std::cout << "Using feet and pounds\n"; }
}

namespace mymath {
    const double PI = 3.14159;
}

int main() {
    metric::describe();      // :: 作用域解析，指定用哪个命名空间的 describe
    imperial::describe();

    std::cout << "PI = " << mymath::PI << "\n";
    return 0;
}
