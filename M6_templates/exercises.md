# M6 练习

> 编译（x64 Native Tools 命令行）：
> ```
> cl /EHsc /std:c++17 /W4 文件名.cpp
> ```
> 模板代码全在头文件里，头文件不单独编译；多文件时命令行只列 `.cpp`。
> 每题先自己写，跑通再看 `answers/`。

难度：⭐ 入门　⭐⭐ 巩固　⭐⭐⭐ 综合

---

## 练习 1 ⭐ 第一个函数模板（对比宏）
**文件：** `ex1_mymax.cpp`

1. 先用 C 的宏写一个 `#define MAX(a, b) ((a) > (b) ? (a) : (b))`。
2. 再写一个函数模板 `template <typename T> T myMax(T a, T b)`。
3. `main` 里：
   - 用 `myMax` 比较两个 int、两个 double。
   - 用宏 `MAX(i++, 5)`（`i` 初值随意），打印 `i`，观察 `i++` 是否被求值了两次。
   - 在注释里写清楚：宏和模板的两个关键区别（类型检查、副作用）。

**练什么：** 函数模板基本语法、模板推导、亲眼看到宏的副作用坑。

---

## 练习 2 ⭐ 显式指定模板参数
**文件：** `ex2_explicit_arg.cpp`

1. 写函数模板 `template <typename T> T half(T x) { return x / 2; }`。
2. `main` 里：
   - `half(10)` 看结果（int 除法，得 5）。
   - `half<double>(10)` 显式指定 `T = double`，看结果（5.0）。
   - `half(10.0)` 让它推导成 double。
3. 注释解释：为什么 `half(10)` 和 `half<double>(10)` 结果不同。

**练什么：** 显式指定模板参数 vs 自动推导，理解 `T` 决定一切。

---

## 练习 3 ⭐⭐ 类模板：泛型 Pair
**文件：** `ex3_pair.cpp`

写一个双类型参数的类模板 `MyPair<A, B>`：
1. 两个公共成员 `first`（A）、`second`（B）。
2. 构造 `MyPair(const A& a, const B& b)`，用初始化列表。
3. 成员函数 `void print() const`，打印 `(first, second)`。
4. `main` 里创建 `MyPair<int, std::string>` 和 `MyPair<double, char>`，各 `print()`。

**练什么：** 多个类型参数的类模板、实例化时指定类型。

---

## 练习 4 ⭐⭐ 非类型模板参数：定长数组
**文件：** `ex4_array.cpp`

写一个类模板 `Array<T, N>`（`N` 是非类型 int 参数）：
1. 私有成员 `T data_[N];`。
2. `T& operator[](int i)` 和 `const T& operator[](int i) const`。
3. `int size() const { return N; }`。
4. `void fill(const T& v)`：把所有元素设为 `v`。
5. `main` 里创建 `Array<int, 5>`，`fill(7)`，用循环打印；再创建 `Array<double, 3>` 验证不同 N 是不同类型。

**练什么：** 非类型模板参数、编译期常量大小、`operator[]` 的 const/非 const 两版。

---

## 练习 5 ⭐⭐ 模板函数与自定义类型
**文件：** `ex5_template_with_class.cpp`

1. 写一个类 `Money`，成员 `cents_`（int），重载 `operator>`（比较金额）和 `operator<<`（友元，打印成 `$x.xx`）。
2. 复用练习 1 的 `myMax` 模板，`myMax(Money{150}, Money{299})` 取较大的。
3. `main` 里演示，打印结果。
4. 注释解释：`myMax` 能作用于 `Money`，是因为 `Money` 提供了 `>`——这就是"实例化时才检查操作是否支持"。

**练什么：** 模板作用于自定义类型的前提（要支持模板内用到的操作），呼应第 7 节。

---

## 练习 6 ⭐⭐ 全特化
**文件：** `ex6_specialization.cpp`

1. 写函数模板 `template <typename T> std::string typeName(T)`，通用版返回 `"unknown"`。
2. 为 `int`、`double`、`const char*` 各写一个全特化，分别返回 `"int"`、`"double"`、`"c-string"`。
3. `main` 里对 `42`、`3.14`、`"hi"` 各调一次，打印结果。

**练什么：** 全特化语法 `template <>`，理解"通用版兜底 + 特化开小灶"。

---

## 练习 7 ⭐⭐⭐ 偏特化 + 类型识别
**文件：** `ex7_partial.cpp`

写一个类模板 `Inspect<T>`，用静态函数 `static std::string kind()` 报告类型属于哪一类：
1. 通用版返回 `"value"`。
2. 偏特化 `Inspect<T*>` 返回 `"pointer"`（匹配任意指针）。
3. 偏特化 `Inspect<T[N]>`（带非类型参数 `N`）返回 `"array"`（匹配任意定长数组）。
4. `main` 里分别用 `int`、`int*`、`double*`、`int[4]` 实例化，打印 `kind()`。

**练什么：** 类模板偏特化、偏特化模板头非空、偏特化能带自己的模板参数。

---

## 练习 8 ⭐⭐⭐ 亲手制造并修复"模板放 .cpp"的链接坑
**文件：** `Adder.h` + `Adder.cpp` + `ex8_main.cpp`

1. `Adder.h`：`#pragma once`，只**声明**函数模板 `template <typename T> T addAll(const T* arr, int n);`（不写函数体）。
2. `Adder.cpp`：`#include "Adder.h"`，写出 `addAll` 的**定义**（累加数组）。
3. `ex8_main.cpp`：`#include "Adder.h"`，`main` 里对一个 int 数组调用 `addAll`。
4. 先按上面这样编译 `cl /EHsc /std:c++17 /W4 ex8_main.cpp Adder.cpp`，观察**链接错误**（`unresolved external symbol` / `undefined reference`）。在注释里记录报错。
5. **修复**：把定义从 `Adder.cpp` 挪回 `Adder.h`（`Adder.cpp` 可留空或删掉对模板的定义），重新编译通过。

> answers 里给的是**修复后**的版本（定义在头文件）。第 4 步的"报错版本"请你自己先制造一次，亲眼看看那个链接错误。

**练什么：** 模板编译模型——为什么定义必须放头文件（第 8 节的核心坑）。

---

## 综合项目 mini ⭐⭐⭐ 泛型 Stack<T>
**文件：** `mini/stack.cpp`

把 M2 那个只装 int 的 `IntStack` 升级成泛型 `Stack<T, N>`（定长，栈上，不用堆内存）：
1. 模板头 `template <typename T, int N = 64>`。
2. 私有成员：`T data_[N];`、`int top_ = 0;`。
3. 成员函数：
   - `bool push(const T& v)`：满了（`top_ == N`）返回 false，否则压入返回 true。
   - `bool pop(T& out)`：空了返回 false，否则弹出到 out 返回 true。
   - `bool empty() const`、`bool full() const`、`int size() const`、`int capacity() const`（返回 N）。
   - `const T& top() const`：看栈顶（不弹）。
4. 重载 `operator<<`（友元模板），打印栈内所有元素（从底到顶）。
5. `main` 里：
   - 用 `Stack<int>`（默认 N=64）压入若干元素，打印，弹出几个再打印。
   - 用 `Stack<std::string, 4>` 压满 4 个，测试第 5 次 `push` 返回 false（满栈边界）。
   - 测试空栈 `pop` 返回 false。

**练什么：** 把本模块全部知识点（类模板、非类型参数、默认模板参数、友元模板、`operator<<`）串成一个能装任意类型的可用容器——这正是 STL 容器的简化原型。

> 提示：友元的 `operator<<` 要访问 `Stack<T, N>` 的私有成员，声明时注意它自己也是个模板。answers 里给出写法。

---

做完告诉我，或对照 `answers/`。这一模块你手写的 `Stack<T>`、`Array<T, N>` 就是 STL 容器的雏形——M7 我们正式**用** STL（`vector`/`map`/`string`/算法/迭代器），你会发现它们不过是"写好了、优化过、装了迭代器"的模板容器。想深入某个点（完美转发、`enable_if`、concepts）随时说，那些是进阶话题。
