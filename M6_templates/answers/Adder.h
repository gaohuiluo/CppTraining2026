// 练习 8：模板编译模型 —— 修复后的版本（定义放在头文件里）
#pragma once

// 关键：模板的定义必须和声明一起放头文件。
// 如果只在这里声明、把定义放进 Adder.cpp，编译 ex8_main.cpp 时看不到定义，
// 无法实例化 addAll<int>，链接会报 "unresolved external symbol / undefined reference"。
template <typename T>
T addAll(const T* arr, int n) {
    T sum = T{};                 // 值初始化：int 得 0，double 得 0.0
    for (int i = 0; i < n; ++i) sum += arr[i];
    return sum;
}
