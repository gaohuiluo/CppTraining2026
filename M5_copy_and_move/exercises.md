# M5 练习

> 编译（x64 Native Tools 命令行）：
> ```
> cl /EHsc /std:c++17 /W4 文件名.cpp
> ```
> 每题先自己写，跑通再看 `answers/`。这一模块务必**多加打印语句**，亲眼看每个特殊成员函数何时被调用——这是理解拷贝/移动最快的路。

难度：⭐ 入门　⭐⭐ 巩固　⭐⭐⭐ 综合

---

## 练习 1 ⭐ 拷贝构造 vs 拷贝赋值，谁在什么时候被调
**文件：** `ex1_when_called.cpp`

写一个最简单的类 `Tracer`（内部存一个 `std::string name_`，不涉及堆指针）：
1. 带参构造 `Tracer(std::string name)`，打印 `[name] 普通构造`。
2. 拷贝构造 `Tracer(const Tracer&)`，打印 `[name] 拷贝构造`。
3. 拷贝赋值 `operator=(const Tracer&)`，打印 `[name] 拷贝赋值`，记得返回 `*this`。
4. 析构，打印 `[name] 析构`。
5. `main` 里依次做这些动作，并在注释里标注每一行触发了哪个函数：
   - `Tracer a("A");`
   - `Tracer b = a;`（这是拷贝构造还是拷贝赋值？）
   - `Tracer c("C");`
   - `c = a;`（这个呢？）
   - 一个按值传参的函数 `void use(Tracer t);`，调用 `use(a);`

**练什么：** 分清「初始化」和「赋值」，看清四种拷贝时机。

---

## 练习 2 ⭐ 亲手制造浅拷贝的崩溃
**文件：** `ex2_shallow_crash.cpp`

写一个持有堆内存的 `Buffer`：私有成员 `int* data_`、`int size_`；构造 `Buffer(int n)` 里 `new int[n]`；析构 `delete[] data_`；构造和析构都打印一句带 `data_` 地址的话。
1. **先不写任何拷贝函数**（用编译器默认的浅拷贝）。
2. `main` 里 `Buffer a(4);`，然后 `Buffer b = a;`，打印 `a` 和 `b` 的 `data_` 地址。
3. 在注释里回答：两个地址是否相同？作用域结束时会发生什么？为什么是 double free？
4. （可选，理解即可，别真跑）说明为什么这段代码语法能过但运行会崩。

**练什么：** 直观感受默认浅拷贝对含指针类的破坏。（本题重点是看地址和推理，不要求真的触发崩溃。）

---

## 练习 3 ⭐⭐ 把浅拷贝改成深拷贝
**文件：** `ex3_deep_copy.cpp`

在练习 2 的 `Buffer` 基础上：
1. 加一个 `set(int i, int v)` 和 `int get(int i) const`，用来读写第 i 个元素。
2. 写**深拷贝构造** `Buffer(const Buffer&)`：分配自己的内存，用 `std::copy` 复制内容。
3. `main` 里：`Buffer a(4)`，填入 1、2、3、4；`Buffer b = a;`（深拷贝）；然后 `b.set(0, 999)`。
4. 打印 `a.get(0)` 和 `b.get(0)`，验证改 `b` 不影响 `a`（说明各持一份内存）。
5. 打印两者 `data_` 地址，确认不同。

**练什么：** 深拷贝构造的正确实现，验证「互不影响」。

---

## 练习 4 ⭐⭐ 正确的拷贝赋值运算符
**文件：** `ex4_copy_assign.cpp`

继续用 `Buffer`（含深拷贝构造），这次补齐**拷贝赋值运算符**，要求四步走：
1. 自赋值检查（`if (this == &other) return *this;`）。
2. 先分配新内存并复制内容。
3. 释放旧内存。
4. 接管新内存、更新 `size_`，返回 `*this`。

`main` 里测试：
- `Buffer a(4)` 填值，`Buffer c(2)` 填别的值。
- `c = a;`（拷贝赋值，`c` 原来的内存应被释放）。
- `c = c;`（自赋值，验证不崩、内容不丢）。
- 打印验证 `c` 现在和 `a` 内容相同、地址不同。

**练什么：** 拷贝赋值的三大陷阱（自赋值、释放旧资源、异常安全顺序）。

---

## 练习 5 ⭐⭐ 左值、右值与右值引用
**文件：** `ex5_value_category.cpp`

不涉及类，专练 value category：
1. 写两个重载函数：`void probe(const int& x)` 打印 `左值`，`void probe(int&& x)` 打印 `右值`。
2. `main` 里分别用下列实参调用 `probe`，先在注释里预测输出，再运行验证：
   - `int a = 5; probe(a);`
   - `probe(10);`
   - `probe(a + 1);`
   - `probe(std::move(a));`
3. 在注释里解释：为什么 `std::move(a)` 会命中右值版本？`std::move` 有没有真的移动 `a`？

**练什么：** 左值/右值直觉、`T&&` 重载、`std::move` 的本质（只是类型转换）。

---

## 练习 6 ⭐⭐⭐ 移动构造与移动赋值
**文件：** `ex6_move.cpp`

给 `Buffer` 补齐**移动构造**和**移动赋值**（此时它已有深拷贝构造、拷贝赋值、析构，凑齐 Rule of 5）：
1. 移动构造 `Buffer(Buffer&&) noexcept`：接管指针，把源对象置空。
2. 移动赋值 `Buffer&& operator=` 版本：释放自己旧资源，接管源资源，置空源对象，防自赋值。
3. 每个特殊成员函数都打印一句（构造/拷贝构造/拷贝赋值/移动构造/移动赋值/析构）。
4. `main` 里对比：
   - `Buffer b = a;`（左值 → 拷贝构造）
   - `Buffer c = std::move(a);`（右值 → 移动构造），打印移动后 `a` 的 `data_` 是否为 `nullptr`。
   - `Buffer d(1); d = std::move(c);`（移动赋值）
5. 注释里回答：移动后 `a` 处于什么状态？还能不能安全析构？

**练什么：** 移动构造/赋值的实现，`noexcept`，移动后源对象「有效但未指定」。

---

## 练习 7 ⭐⭐ =default / =delete 与 NonCopyable
**文件：** `ex7_default_delete.cpp`

1. 写一个 `NonCopyable` 类：默认构造 `=default`，拷贝构造和拷贝赋值 `=delete`。
2. 在 `main` 里写一行会被 `=delete` 挡住的拷贝（如 `NonCopyable b = a;`），**注释掉**它并说明为什么编译不过。
3. 写一个 `Trivial` 类，成员是几个 `int` 和一个 `std::string`，把五个特殊成员函数**全部 `=default`**，说明这等价于什么（提示：等价于一个都不写，即 Rule of 0）。
4. 用注释解释：为什么 `std::unique_ptr` 的拷贝是被 `=delete` 的，而移动是允许的？

**练什么：** `=default`/`=delete` 的用法，独占语义，和 Rule of 0 的联系。

---

## 练习 8 ⭐⭐⭐ Rule of 0：用 vector 让编译器代劳
**文件：** `ex8_rule_of_zero.cpp`

把管理堆内存的 `Buffer` 重写成 **Rule of 0** 版本：
1. 用 `std::vector<int> data_` 作为唯一数据成员，**不写任何特殊成员函数**（不写析构、拷贝、移动）。
2. 提供构造 `Buffer(int n)`、`set`/`get`、`size()`。
3. `main` 里验证它**能拷贝也能移动**：
   - `Buffer b = a;`（编译器默认拷贝 = vector 深拷贝，改 b 不影响 a）。
   - `Buffer c = std::move(a);`（编译器默认移动 = vector 移动）。
4. 注释里对比：和练习 6 手写 Rule of 5 的版本相比，这个版本少写了多少代码？为什么它依然安全（谁在负责深拷贝和释放）？

**练什么：** Rule of 0 的实战写法，理解「把资源交给懂事的成员类型」。

---

## 综合项目 mini ⭐⭐⭐ 完整 Rule of 5 的 Buffer + 调用观察
**文件：** `mini/buffer.cpp`

实现一个**完整遵守 Rule of 5** 的 `Buffer` 类（持有堆上的 `int` 数组），把六个函数（普通构造 + 五个特殊成员函数）全部手写，每个都用打印语句标记，然后写测试观察它们何时被调用。

要求：
1. 私有成员：`int* data_`、`std::size_t size_`。
2. **普通构造** `Buffer(std::size_t n)`：`new int[n]`，元素清零，打印 `构造(size=n) @地址`。
3. **析构** `~Buffer()`：`delete[] data_`，打印 `析构 @地址`。
4. **拷贝构造** `Buffer(const Buffer&)`：深拷贝，打印 `拷贝构造`。
5. **拷贝赋值** `operator=(const Buffer&)`：四步走（防自赋值、分配、释放旧、接管），打印 `拷贝赋值`。
6. **移动构造** `Buffer(Buffer&&) noexcept`：接管 + 置空源，打印 `移动构造`。
7. **移动赋值** `operator=(Buffer&&) noexcept`：防自赋值、释放旧、接管、置空源，打印 `移动赋值`。
8. 辅助：`set`/`get`/`size()`，以及 `operator<<`（友元，打印全部元素，回忆 M2）。
9. `main` 里设计一段测试脚本，逐一触发并用注释标注预期：
   - 普通构造 `Buffer a(4)`，填入数据。
   - 拷贝构造 `Buffer b = a;`。
   - 拷贝赋值 `Buffer c(2); c = a;`。
   - 移动构造 `Buffer d = std::move(a);`（观察 `a` 被掏空）。
   - 移动赋值 `Buffer e(1); e = std::move(b);`。
   - 一个按值返回的工厂函数 `Buffer make(std::size_t n);`，用 `Buffer f = make(5);` 观察 RVO（八成一次多余的构造都看不到）。
   - 把几个 `Buffer` `push_back` 进 `std::vector<Buffer>`，观察扩容时用的是移动还是拷贝（因为标了 `noexcept`）。
10. 在文件末尾用一大段注释总结：每种操作各调用了哪个特殊成员函数，以及 RVO 为什么让你看不到返回时的拷贝/移动。

**练什么：** 把整个 M5 串起来——Rule of 5 全实现、`noexcept`、`std::move`、RVO、vector 扩容用移动，一次打通。

---

做完对照 `answers/`。真正学到东西的标志是：你能**预测**每一行会打印什么，运行结果和你的预测一致。如果某处和预期不符（尤其是 RVO 和 vector 扩容那两处），回去重读 principles 第 9-11 节。
