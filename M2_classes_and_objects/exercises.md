# M2 练习

> 编译（x64 Native Tools 命令行）：
> ```
> cl /EHsc /std:c++17 /W4 文件名.cpp
> ```
> 每题先自己写，跑通再看 `answers/`。

难度：⭐ 入门　⭐⭐ 巩固　⭐⭐⭐ 综合

---

## 练习 1 ⭐ 第一个类
**文件：** `ex1_point.cpp`

写一个 `Point` 类：
1. 两个私有成员 `x_`、`y_`（int）。
2. 带参构造 `Point(int x, int y)`，**用初始化列表**。
3. 公共 const 成员函数 `getX()`、`getY()`。
4. 公共 const 成员函数 `print()`，打印 `(x, y)`。
5. `main` 里创建一个 `Point(3, 4)` 并 `print()`。

**练什么：** 类骨架、访问控制、初始化列表、const 成员函数。

---

## 练习 2 ⭐ 封装与校验
**文件：** `ex2_temperature.cpp`

写一个 `Temperature` 类，内部只存摄氏度 `celsius_`：
1. `setCelsius(double c)`：如果 `c < -273.15`，则夹到 `-273.15`（绝对零度）。
2. `getCelsius() const`、`getFahrenheit() const`（摄氏转华氏：`c*9/5+32`）。
3. `main` 里设一个非法值（如 `-300`），打印摄氏和华氏，验证被夹住了。

**练什么：** 封装的意义——私有数据 + 受控 setter 做校验。

---

## 练习 3 ⭐⭐ 构造 + 析构（看生命周期）
**文件：** `ex3_lifetime.cpp`

写一个 `Logger` 类：
1. 带参构造 `Logger(std::string name)`，构造时打印 `[name] 构造`。
2. 析构时打印 `[name] 析构`。
3. 在 `main` 里：
   - 创建 `Logger a("A");`
   - 用一对 `{ }` 开一个作用域，在里面创建 `Logger b("B");`
   - 观察输出顺序，在注释里解释：为什么 B 先于 A 析构？

**练什么：** 构造/析构的自动调用时机、作用域与生命周期、析构逆序。

---

## 练习 4 ⭐⭐ 静态成员做对象计数
**文件：** `ex4_counter.cpp`

写一个 `Widget` 类，统计当前存活的对象数量：
1. 静态成员 `count_`（用 C++17 的 `inline static int count_ = 0;`）。
2. 构造时 `++count_`，析构时 `--count_`。
3. 静态成员函数 `static int alive()` 返回当前数量。
4. `main` 里创建/销毁若干个 `Widget`（配合 `{ }` 作用域），在关键点打印 `Widget::alive()` 验证。

**练什么：** 静态成员变量（共享）、静态成员函数、结合生命周期。

---

## 练习 5 ⭐⭐ explicit 的作用
**文件：** `ex5_explicit.cpp`

1. 写一个类 `Buffer`，带单参数构造 `Buffer(int size)`，构造时打印 `分配 size 字节`。
2. 写一个函数 `void use(const Buffer& b)`。
3. **先不加 `explicit`**，试 `use(100);`，看它能编译（发生了隐式转换）。
4. 给构造函数加上 `explicit`，再试 `use(100);`，看编译报错；改成 `use(Buffer(100));` 修复。
5. 注释里写清楚 explicit 前后的差别。

**练什么：** `explicit` 如何阻止意外的隐式转换。

---

## 练习 6 ⭐⭐ this 与链式调用
**文件：** `ex6_chain.cpp`

写一个 `StringBuilder` 类，内部存一个 `std::string result_`：
1. `append(const std::string& s)`：把 s 接到 result_ 后面，**返回 `*this`**（返回类型 `StringBuilder&`）。
2. `str() const`：返回 result_。
3. `main` 里链式调用：`sb.append("Hello").append(", ").append("World").append("!");` 然后打印 `sb.str()`。

**练什么：** `this`、返回 `*this` 实现链式调用。

---

## 练习 7 ⭐⭐⭐ 运算符重载：Vec2
**文件：** `ex7_vec2.cpp`

写一个二维向量 `Vec2`（成员 `x_`、`y_` 为 double）：
1. 构造 `Vec2(double x = 0, double y = 0)`。
2. 重载 `operator+`（成员函数，向量相加）。
3. 重载 `operator*`（成员函数，向量乘标量：`v * 2.0`）。
4. 重载 `operator==`（成员函数）。
5. 重载 `operator<<`（**友元非成员函数**），打印成 `(x, y)`。
6. `main` 里演示：`a + b`、`a * 3.0`、`a == b`，并用 `std::cout << ...` 打印结果。

**练什么：** 运算符重载的成员形式与友元非成员形式，`operator<<` 的写法。

---

## 练习 8 ⭐⭐⭐ 头文件/源文件拆分
**文件：** `Fraction.h` + `Fraction.cpp` + `ex8_main.cpp`

把一个"分数"类拆成三个文件：
1. `Fraction.h`：用 `#pragma once`，声明类 `Fraction`——私有成员 `num_`、`den_`（分子/分母）；构造 `Fraction(int num, int den)`；`operator+`；`operator<<`（友元）；`double toDouble() const`。
2. `Fraction.cpp`：实现全部成员（构造里可选做约分，用 `gcd`；不做也行）。
3. `ex8_main.cpp`：创建两个分数相加并打印，打印其 double 值。

编译：`cl /EHsc /std:c++17 /W4 ex8_main.cpp Fraction.cpp`

**练什么：** 声明/实现分离、`#pragma once`、类外定义 `类名::` 语法、多文件编译。

---

## 综合项目 mini ⭐⭐⭐
**文件：** `mini/stack.cpp`

自己实现一个**定长整数栈** `IntStack`（这题会为 M4/收尾项目打基础）：
1. 私有成员：`int data_[64];`、`int top_;`。
2. 构造函数初始化 `top_ = 0`。
3. 成员函数：
   - `bool push(int v)`：满了（top_ == 64）返回 false，否则压入返回 true。
   - `bool pop(int& out)`：空了返回 false，否则弹出到 out 返回 true。
   - `bool empty() const`、`int size() const`。
4. 重载 `operator<<`（友元），打印栈内所有元素（从底到顶）。
5. `main` 里压入若干元素，打印，弹出几个，再打印，测试空栈/满栈边界。

**练什么：** 把 M2 全部知识点（封装、构造、const 函数、边界处理、运算符重载）串成一个可用的小组件。

---

做完告诉我，或对照 `answers/`。想让我深入讲某个点（比如运算符重载的其他形式、拷贝时会发生什么）随时说——注意"对象拷贝会发生什么"正是 M5 的核心，这里先埋个伏笔。
