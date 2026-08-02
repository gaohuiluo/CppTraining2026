# M4 练习

> 编译（x64 Native Tools 命令行）：
> ```
> cl /EHsc /std:c++17 /W4 文件名.cpp
> ```
> 每题先自己写，跑通再看 `answers/`。

难度：⭐ 入门　⭐⭐ 巩固　⭐⭐⭐ 综合

---

## 练习 1 ⭐ 栈 vs 堆，new/delete 初体验
**文件：** `ex1_stack_vs_heap.cpp`

写一个带构造/析构打印的类 `Tracer`（构造打印 `Tracer(name) 构造`，析构打印 `Tracer(name) 析构`）：
1. 在 `main` 里创建一个**栈对象** `Tracer s("stack")`。
2. 用 `new` 创建一个**堆对象** `Tracer* h = new Tracer("heap");`。
3. 在 `main` 结束前**手动** `delete h;`。
4. 观察输出，在注释里回答：如果不写 `delete h`，堆对象的析构会被调用吗？栈对象呢？

**练什么：** 栈/堆生命周期差异，`new`/`delete` 会调用构造/析构，泄漏的直观感受。

---

## 练习 2 ⭐ new/delete vs malloc/free
**文件：** `ex2_new_vs_malloc.cpp`

写一个类 `Widget`，构造函数里 `std::cout << "构造\n";`，析构里 `std::cout << "析构\n";`：
1. 用 `Widget* a = new Widget();` 创建，再 `delete a;`——观察构造/析构都被调用。
2. 用 `Widget* b = (Widget*)std::malloc(sizeof(Widget));` 创建，再 `std::free(b);`——观察构造/析构**都没被调用**（`malloc` 只给内存）。
3. 在注释里总结 `new`/`delete` 和 `malloc`/`free` 的核心区别（构造析构、类型、sizeof、失败处理）。

**练什么：** 亲眼看到 `new` 调构造、`malloc` 不调，理解为什么有资源的类不能用 malloc。

---

## 练习 3 ⭐⭐ 把内存错误亲手踩一遍
**文件：** `ex3_memory_bugs.cpp`

这题是「反面教材」——写出四种错误，然后**在注释里写出正确版本**（错误代码用注释包起来避免真的崩溃/泄漏，能编译即可）：
1. 内存泄漏：`new` 了不 `delete`。
2. 悬垂指针：`delete` 后继续解引用。
3. double free：同一指针 `delete` 两次。
4. `new[]` / `delete` 不匹配。

每种错误下面用注释写：为什么错、怎么改对。`main` 里只保留能安全编译运行的正确版本，错误示例放注释里。

**练什么：** 把四类经典内存错误理解透，为智能指针铺垫。

---

## 练习 4 ⭐⭐ 手写一个 RAII 类：FileGuard
**文件：** `ex4_raii_file.cpp`

用 RAII 封装 C 的 `FILE*`：
1. 类 `FileGuard`：构造 `FileGuard(const char* path, const char* mode)`，内部 `std::fopen`。
2. 析构里：如果文件指针非空，`std::fclose` 并打印 `文件已关闭`。
3. 提供 `bool valid() const`（文件是否成功打开）、`std::FILE* get() const`。
4. `main` 里：创建一个 `FileGuard` 写一行字到临时文件，然后**什么都不用做**——观察离开作用域时自动关闭。
5. 注释里对比：C 里要手动 `fclose`，且每条退出路径都要记得；RAII 只写一次。

**练什么：** RAII 的核心模式，对比 C 手动配对 fopen/fclose 的痛。

---

## 练习 5 ⭐⭐ unique_ptr 基础
**文件：** `ex5_unique_ptr.cpp`

1. 定义类 `Resource`（构造/析构各打印一句）。
2. 用 `std::make_unique<Resource>()` 创建一个 `unique_ptr`，用 `->` 调用它的方法。
3. 演示 `unique_ptr` **不能拷贝**：写一行 `auto b = a;` 并**注释掉**，注释说明为什么编译不过。
4. 演示**可以移动**：`auto c = std::move(a);`，之后检查 `a` 是否变空（`if (!a) std::cout << "a 现在是空\n";`）。
5. 观察离开作用域自动析构，全程没有 `delete`。

**练什么：** `make_unique`、独占语义、不可拷贝可移动、自动释放。

---

## 练习 6 ⭐⭐ unique_ptr 工厂函数
**文件：** `ex6_factory.cpp`

1. 定义一个抽象基类 `Shape`（纯虚 `double area() const`，**虚析构** `virtual ~Shape()`）。
2. 派生 `Circle`、`Rectangle`。
3. 写工厂函数 `std::unique_ptr<Shape> makeShape(...)`，根据参数返回不同派生类对象。
4. `main` 里用工厂创建几个 `Shape`，调用 `area()`，观察离开作用域时**通过基类指针也能正确析构**（因为虚析构）。

**练什么：** `unique_ptr` 做工厂返回值、所有权转移、虚析构 + 智能指针（呼应 M3）。

---

## 练习 7 ⭐⭐ shared_ptr 与引用计数
**文件：** `ex7_shared_ptr.cpp`

1. 定义类 `Resource`（构造/析构打印）。
2. 用 `std::make_shared<Resource>()` 创建 `a`，打印 `a.use_count()`。
3. 在内层作用域 `{ }` 里 `auto b = a;`，打印计数（应为 2）。
4. 内层结束后再打印计数（应回到 1）。
5. 观察：只有当**最后一个** shared_ptr 销毁时，`Resource` 才析构。

**练什么：** `make_shared`、引用计数随拷贝/销毁增减、计数归零才释放。

---

## 练习 8 ⭐⭐⭐ 循环引用与 weak_ptr 修复
**文件：** `ex8_weak_ptr.cpp`

1. 定义 `struct Node`，含 `std::shared_ptr<Node> next;`、`std::shared_ptr<Node> prev;`，析构打印 `Node 析构`。
2. **第一部分（泄漏）**：写函数 `leak()`，创建两个 Node 用 shared_ptr 互指，观察离开作用域后**看不到「Node 析构」**——泄漏了。
3. **第二部分（修复）**：把 `prev` 改成 `std::weak_ptr<Node>`，重跑，观察这次**能看到「Node 析构」**。
4. 演示 `weak_ptr` 的用法：用 `lock()` 从 `prev` 拿回 shared_ptr 再访问。
5. 注释解释：为什么 weak_ptr 能打破循环。

**练什么：** 循环引用泄漏的成因、weak_ptr 破环、`lock()` 用法。（本题可把两部分分成两个函数放同一文件，注意 struct 定义只能有一份，可用两个不同的 struct 名，如 `BadNode`/`GoodNode`。）

---

## 综合项目 mini ⭐⭐⭐
**文件：** `mini/my_unique_ptr.cpp`（主）、`mini/raii_file.cpp`（配套小练，二选一或都做）

### 主项目：手写简化版 `UniquePtr<T>`
自己实现一个能用的独占智能指针 `UniquePtr<T>`，彻底理解 unique_ptr 原理：
1. 模板类 `UniquePtr<T>`，内部持有 `T* ptr_`。
2. 构造：`explicit UniquePtr(T* p = nullptr)`。
3. 析构：`delete ptr_;`（自动释放——RAII 的核心）。
4. **禁止拷贝**：`UniquePtr(const UniquePtr&) = delete;` 和 `operator=(const UniquePtr&) = delete;`（独占语义）。
5. **支持移动**：移动构造和移动赋值——把对方的 `ptr_` 偷过来，对方置空（`= delete` 拷贝、`= default` 不行，要手写移动，把源指针置 nullptr）。
6. 重载 `operator*`、`operator->`，让它用起来像指针。
7. 提供 `T* get() const`、`explicit operator bool() const`（判空）、`void reset(T* p = nullptr)`、`T* release()`。
8. `main` 里用你的 `UniquePtr` 管理一个带构造/析构打印的类，演示：自动释放、移动转移所有权、`->` 访问。

> 提示：移动赋值里记得先释放自己原来持有的资源，再接管对方的，并防自赋值。这是「资源管理类」的通用套路，M5 会正式讲。

**练什么：** 从零实现 RAII + 独占语义 + 移动，把智能指针的原理吃透。

### 配套小练：RAII 封装 FILE*（`raii_file.cpp`）
把练习 4 的 `FileGuard` 升级：
1. 支持读/写模式，提供 `writeLine(const std::string&)`、`readAll()` 等便捷方法。
2. **禁止拷贝**（两个对象持同一个 FILE* 会 double close），可选支持移动。
3. `main` 演示写文件再读回，全程无需手动 fclose。

**练什么：** 把 RAII 用在真实资源（文件句柄）上，理解「为什么资源类通常要禁止拷贝」。

---

做完告诉我，或对照 `answers/`。想深入某个点随时说——特别是 mini 项目里手写的「移动构造/移动赋值」，`std::move` 到底做了什么、为什么资源类要 `= delete` 拷贝，**这些正是 M5「拷贝与移动语义、Rule of 0/3/5」的核心**，这里先埋个伏笔。
