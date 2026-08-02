# M1 练习

> 用法：每题先自己写，编译跑通再看 `answers/` 对照。编译命令（x64 Native Tools 命令行里）：
> ```
> cl /EHsc /std:c++17 /W4 文件名.cpp
> ```

难度标记：⭐ 入门　⭐⭐ 巩固　⭐⭐⭐ 综合

---

## 练习 1 ⭐ Hello, iostream
**文件：** `ex1_hello.cpp`

写一个程序：
1. 用 `std::cout` 打印 `Hello, C++!` 并换行。
2. 定义 `int year = 2026;`，打印一行 `Year: 2026`。
3. 提问：如果把 `"\n"` 换成 `std::endl`，行为有什么不同？（写在注释里）

**练什么：** `iostream`、`std::cout`、`<<` 链式、`std::` 前缀。

---

## 练习 2 ⭐ printf 改 iostream
**文件：** `ex2_convert.cpp`

下面是一段 C 代码，把它改写成地道 C++（用 `iostream`，不许用 `printf`）：

```c
#include <stdio.h>
int main(void) {
    int    n     = 5;
    double price = 3.14;
    char   grade = 'A';
    printf("n=%d price=%.2f grade=%c\n", n, price, grade);
    return 0;
}
```

**练什么：** 输出流对比，体会"不用占位符"。
提示：小数默认打印格式和 `%.2f` 不完全一样，先不用纠结精度控制，能打印出来即可。

---

## 练习 3 ⭐ 引用做 swap
**文件：** `ex3_swap.cpp`

1. 写一个用**指针**的 `swap_ptr(int* a, int* b)`（C 风格）。
2. 写一个用**引用**的 `swap_ref(int& a, int& b)`（C++ 风格）。
3. 在 `main` 里各调用一次，打印交换前后的值，对比两种调用写法的差异。

**练什么：** 引用 vs 指针，函数传参。

---

## 练习 4 ⭐⭐ const 引用传参
**文件：** `ex4_constref.cpp`

1. 写函数 `void printInfo(const std::string& name, int age);`，打印 `name (age岁)`。
2. 在 `main` 里定义一个 `std::string` 和 `int`，调用它。
3. 试着在 `printInfo` 内部写一句 `name += "x";`，编译看报什么错，然后删掉。把你看到的错误信息摘要写进注释。

**练什么：** `const T&` 只读传参，理解 const 的约束力。

---

## 练习 5 ⭐⭐ 指针 const 三兄弟
**文件：** `ex5_const.cpp`

定义 `int x = 1, y = 2;`，然后依次尝试下面每一行，**能编译的留下、不能编译的注释掉并写明原因**：

```cpp
const int* p1 = &x;
// p1 = &y;      // ?
// *p1 = 10;     // ?

int* const p2 = &x;
// p2 = &y;      // ?
// *p2 = 10;     // ?

const int* const p3 = &x;
// p3 = &y;      // ?
// *p3 = 10;     // ?
```

把每个 `// ?` 换成"能/不能 + 为什么"。

**练什么：** `const` 位置的含义，从右往左读的口诀。

---

## 练习 6 ⭐⭐ 范围 for + 修改
**文件：** `ex6_rangefor.cpp`

1. 定义 `int arr[6] = {10, 20, 30, 40, 50, 60};`。
2. 用范围 for（只读、`const auto&`）打印所有元素。
3. 用范围 for（引用 `int&`）把每个元素加上 5。
4. 再打印一次验证。

**练什么：** 范围 for 的三种写法（值 / 引用 / const 引用）。

---

## 练习 7 ⭐⭐ std::string 练手
**文件：** `ex7_string.cpp`

用 `std::string` 完成（不许用 `char[]` 和 `<cstring>`）：
1. 读入用户输入的名字（`std::cin >> name;`）。
2. 拼接成 `Hello, <名字>!`。
3. 打印这个问候语，以及名字的长度（`.size()`）。
4. 打印名字的第一个字符（`name[0]` 或 `name.at(0)`）。

**练什么：** `std::string` 基本操作，对比 C 的字符串处理。

---

## 练习 8 ⭐⭐⭐ std::vector 综合
**文件：** `ex8_vector.cpp`

不用裸数组、不用 `malloc`，用 `std::vector<int>` 完成：
1. 让用户输入若干个整数，输入 `-1` 结束（用循环 + `push_back`）。
2. 打印元素个数。
3. 用范围 for 求和并打印。
4. 找出最大值并打印。

**练什么：** `vector` 动态增长、遍历，替代 C 的手动内存管理。

---

## 练习 9 ⭐⭐⭐ 命名空间 + 综合
**文件：** `ex9_namespace.cpp`

1. 定义两个命名空间 `metric` 和 `imperial`，各写一个函数 `describe()`：
   - `metric::describe()` 打印 `Using meters and kilograms`
   - `imperial::describe()` 打印 `Using feet and pounds`
2. 在 `main` 里都调用一次。
3. 定义一个 `const double PI = 3.14159;` 放在自己的命名空间 `mymath` 里，在 `main` 里用 `mymath::PI` 打印它。

**练什么：** 命名空间定义与访问、`::` 运算符。

---

## 挑战题 ⭐⭐⭐（可选）
**文件：** `ex10_challenge.cpp`

把下面这段"很 C"的代码，整体重写成地道的现代 C++（用 `std::vector`、`std::string`、范围 for、引用、`const`，去掉所有裸指针和固定数组）：

```c
#include <stdio.h>
#include <string.h>

#define MAX 100

int main(void) {
    int  scores[MAX];
    int  count = 0;
    int  n;
    printf("How many scores? ");
    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        scanf("%d", &scores[i]);
        count++;
    }
    int sum = 0;
    for (int i = 0; i < count; i++) sum += scores[i];
    printf("Average: %.2f\n", (double)sum / count);
    return 0;
}
```

**练什么：** 把 M1 全部知识点串起来，完成一次真正的 "C → C++" 迁移。

---

做完后告诉我，或直接对照 `answers/`。有任何一题卡住、或想让我讲透某个点，随时说。
