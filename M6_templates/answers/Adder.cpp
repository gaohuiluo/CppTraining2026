// 练习 8：修复后 Adder.cpp 里不再放模板定义（定义已挪进 Adder.h）。
// 这个文件保留只是为了对照编译命令；它本身没有内容可编译。
// 编译整体：cl /EHsc /std:c++17 /W4 ex8_main.cpp Adder.cpp
#include "Adder.h"

// 这里故意留空：模板定义放头文件即可。
// 反面教材（会导致链接错误）是把 addAll 的函数体写在这里、头文件只留声明。
