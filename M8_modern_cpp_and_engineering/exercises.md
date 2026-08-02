# M8 练习

> 编译（x64 Native Tools 命令行）：
> ```
> cl /EHsc /std:c++17 /W4 文件名.cpp
> ```
> 用 CMake 的题目见各自说明。每题先自己写，跑通再看 `answers/`。

难度：⭐ 入门　⭐⭐ 巩固　⭐⭐⭐ 综合

---

## 练习 1 ⭐ optional 表达「可能没有值」
**文件：** `ex1_optional.cpp`

写一个函数 `std::optional<int> findFirstEven(const std::vector<int>& v)`，返回第一个偶数；没有偶数就返回 `std::nullopt`。
1. 在 `main` 里对「有偶数」和「全奇数」两个 vector 各调用一次。
2. 有值时用 `if (opt)` 判断并打印 `*opt`；也演示一次 `value_or(-1)`。
3. 注释里写清楚：对空 optional 用 `*opt` 会怎样，为什么要先判断。

**练什么：** `optional` 的返回、判断、取值、`value_or`。

---

## 练习 2 ⭐⭐ variant + visit 类型安全联合
**文件：** `ex2_variant.cpp`

定义 `using Value = std::variant<int, double, std::string>;`
1. 写一个 visitor（struct，三个 `operator()`），分别打印不同类型的值。
2. 建一个 `std::vector<Value>`，塞入 int、double、string 各若干。
3. 遍历并对每个元素 `std::visit(visitor, x)`。
4. 额外演示：用 `std::holds_alternative` 和 `std::get_if` 各判断/取值一次。

**练什么：** `variant` 存取、`visit` 分派、`holds_alternative`/`get_if`。

---

## 练习 3 ⭐⭐ string_view 与悬垂
**文件：** `ex3_string_view.cpp`

1. 写函数 `bool startsWith(std::string_view s, std::string_view prefix)`，判断 s 是否以 prefix 开头（用 `substr`/`size` 比较，不拷贝）。
2. `main` 里用 `std::string`、字符串字面量分别调用它，验证都不拷贝。
3. 在注释里写出一个**悬垂的错误例子**（比如返回指向局部 string 的 view），并说明为什么危险、正确做法是什么。（错误例子写在注释里即可，别真的触发 UB。）

**练什么：** `string_view` 当参数、`substr` 不拷贝、悬垂风险认知。

---

## 练习 4 ⭐ 结构化绑定遍历 map
**文件：** `ex4_structured_binding.cpp`

1. 建一个 `std::map<std::string, int>`，放几组「名字 -> 分数」。
2. 用 `for (const auto& [name, score] : m)` 遍历打印。
3. 再演示一次拆 pair：让一个函数返回 `std::pair<bool, int>`，用 `auto [ok, val] = ...` 接收。
4. 注释说明 `auto&` / `const auto&` / `auto` 在遍历时的区别。

**练什么：** 结构化绑定拆 map / pair，引用 vs 拷贝。

---

## 练习 5 ⭐⭐ constexpr 编译期计算
**文件：** `ex5_constexpr.cpp`

1. 写一个 `constexpr` 函数 `factorial(int n)`（递归或循环都行）。
2. 用 `constexpr int f5 = factorial(5);` 让它在编译期算出来。
3. 用 `factorial(5)` 当数组大小：`int arr[factorial(4)];`（验证能当编译期常量）。
4. 再传一个运行期变量给它，说明此时退化成普通函数调用。
5. 注释对比：这比 C 的 `#define` 宏好在哪。

**练什么：** `constexpr` 变量/函数、编译期 vs 运行期求值。

---

## 练习 6 ⭐⭐ enum class
**文件：** `ex6_enum_class.cpp`

1. 定义 `enum class Direction { North, East, South, West };`
2. 写函数 `std::string toString(Direction d)`（用 switch）。
3. 演示：`Direction d = Direction::North;`，不能隐式转 int，要转得 `static_cast<int>(d)`。
4. 定义另一个 `enum class Status { North, Ok };`，说明 `North` 不和 `Direction::North` 冲突（作用域隔离）。
5. 注释写清相比裸 enum 的好处。

**练什么：** `enum class` 强类型、作用域、`static_cast`。

---

## 练习 7 ⭐⭐ 异常与 RAII 异常安全
**文件：** `ex7_exceptions.cpp`

1. 写一个 RAII 类 `Guard`：构造打印 `获取资源`，析构打印 `释放资源`。
2. 写函数 `void risky(bool fail)`：函数里先建一个 `Guard g;`，若 `fail` 为真则 `throw std::runtime_error("boom");`。
3. `main` 里 `try { risky(true); } catch (const std::exception& e) { ... }`。
4. 观察输出：即使抛异常，`Guard` 的「释放资源」也照常打印。注释解释为什么（栈展开时析构照常调用）。
5. 额外：写一个 `[[nodiscard]]` 函数，演示忽略返回值会有告警。

**练什么：** `throw`/`try`/`catch`、栈展开、RAII 异常安全、`[[nodiscard]]`。

---

## 练习 8 ⭐⭐ thread + mutex 保护计数器
**文件：** `ex8_thread_mutex.cpp`

1. 一个全局 `long counter = 0;` 和一个 `std::mutex`。
2. 写函数 `void addN(int n)`：循环 n 次，每次在 `lock_guard` 保护下 `++counter`。
3. `main` 里开两个线程各跑 `addN(100000)`，`join` 后打印 `counter`，应恰好是 200000。
4. 注释里说明：如果去掉锁会发生数据竞争，结果会小于 200000 且每次不同。

**练什么：** `std::thread`、`join`、`mutex` + `lock_guard`、数据竞争认知。

---

## 练习 9 ⭐⭐ atomic vs mutex
**文件：** `ex9_atomic.cpp`

1. 用 `std::atomic<long> counter{0};` 重做练习 8 的计数（不用锁）。
2. 两个线程各自 `++counter` 十万次，`join` 后验证结果为 200000。
3. 注释对比：这里为什么 atomic 就够了、什么情况下必须用 mutex（多步操作的一致性）。

**练什么：** `std::atomic` 入门、atomic 与 mutex 的适用边界。

---

## 练习 10 ⭐⭐ 零依赖单元测试
**文件：** `ex10_unittest.cpp`

1. 写一个 `CHECK(cond)` 宏（失败打印表达式 + 文件行号，累加失败计数）。
2. 写几个被测函数（如 `int clamp(int v, int lo, int hi)`）。
3. 在 `main` 里对边界、正常值写若干 `CHECK`。
4. 全通过返回 0，有失败返回非零（并打印失败数）。
5. 故意留一个会失败的 CHECK（注释掉或用注释说明），体会「失败要返回非零退出码」。

**练什么：** 单测思路、断言宏、退出码约定。

---

## 综合项目 mini ⭐⭐⭐ 线程安全的生产者-消费者队列
**目录：** `mini/`

用 `std::thread` + `std::mutex` + `std::condition_variable` 实现一个**线程安全的有界任务队列**，跑一个生产者-消费者场景，配一个最小 `CMakeLists.txt` 和零依赖断言测试。

要求：
1. **`ThreadSafeQueue`**（放 `tsqueue.h`，头文件即可，模板类 `template <typename T>`）：
   - `void push(T item)`：加锁入队，`notify_one` 唤醒消费者。
   - `T waitAndPop()`：用 `unique_lock` + 带谓词的 `cv.wait`，队列空就睡，有数据才取。
   - `bool empty() const`（加锁）。
   - 用一个 `std::atomic<bool> done_` 或计数配合，支持「生产结束后消费者能优雅退出」。
2. **`main.cpp`**：
   - 开 1 个生产者线程 push 若干整数任务（比如 0..99）。
   - 开 2 个消费者线程，各自 `waitAndPop` 处理任务，用一个 `std::atomic<long>` 累加「处理了多少个 / 求和」。
   - 生产完发结束信号，消费者收到就退出。
   - `join` 全部线程，打印总处理数、总和，验证 = 期望值（100 个，和为 0+..+99=4950）。
3. **`tests.cpp`**：零依赖 `CHECK` 断言，单线程测 `push`/`waitAndPop` 的基本正确性（入队顺序、数量）。
4. **`CMakeLists.txt`**：
   - C++17，两个可执行目标：`pc_demo`（main）、`pc_tests`（tests）。
   - `enable_testing()` + `add_test` 注册 tests。
   - 链接线程库（`find_package(Threads REQUIRED)` + `Threads::Threads`）。
5. 构建运行：
   ```
   cmake -S . -B build
   cmake --build build
   ctest --test-dir build
   ```

**练什么：** 把并发三件套（thread/mutex/condition_variable）、atomic、RAII 锁、模板、CMake、单测串成一个能跑的真实小组件。这是本模块的集大成练习，也是面试里聊「你写过什么并发代码」的好素材。

---

做完对照 `answers/`。想深入某个点（比如 `std::async`/`future`、无锁队列、`std::expected`、GoogleTest 实操）随时说。

> 注：`answers/` 里所有 `.cpp` 都通过了 C++17 语法检查（本机 g++ `-fsyntax-only` / `-c`，线程题加 `-pthread`）。沙箱不能链接生成 exe，你在本机 MSVC / CMake 下按上面命令即可完整构建运行。
