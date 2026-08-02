# M8 现代特性与工程化（完整）

> 目标：把「现代 C++（C++11/14/17）里工作最常用、面试最高频」的语言特性和工程实践一次补齐。前面 M1-M7 你已经把类、继承、RAII、拷贝移动、模板、STL 打通了；这一模块换个视角，讲的不是「某个语法点」，而是**你上手真实项目立刻要用的东西**：怎么优雅地表达「可能没有值」、怎么做类型安全的联合体、怎么处理错误、怎么写多线程、怎么用 CMake 把代码组织成工程、怎么测试和排错。原理讲到够用即止，重点是建立正确心智模型 + 记住高频面试答案。

---

## 0. 一句话总览

**现代 C++ 做的事，本质是把 C 里「靠约定、靠小心、靠文档」的东西，变成「靠类型系统、靠编译器、靠 RAII」强制保证的东西。**

- 「可能没有值」→ 不再用 `-1`/`NULL`/输出参数，用 `std::optional`。
- 「多种类型之一」→ 不再用裸 `union` + tag，用 `std::variant`。
- 「只读字符串参数」→ 不再传 `const char*` + 长度，用 `std::string_view`。
- 「错误处理」→ 除了错误码，多了异常这条路，配 RAII 做到异常安全。
- 「并发」→ 标准库直接给你 `thread`/`mutex`/`atomic`，锁也是 RAII 的。
- 「工程化」→ CMake 管构建、单测保正确、Sanitizer 抓内存 bug。

记住这条主线：**现代特性不是炫技，是把「运行期才炸」的问题提前到「编译期就拦」。**

---

## 第一部分：现代语言特性

## 1. `std::optional`：优雅地表达「可能没有值」

### 1.1 C 里怎么表达「查不到 / 算不出」

C 程序员有三招，都难受：

```c
// 招式一：用一个"不可能的"返回值当哨兵
int find_index(const int* arr, int n, int target); // 找不到返回 -1
// 问题：如果 -1 是合法下标呢？如果返回值是任意 int 呢？没有"不可能的值"可选

// 招式二：输出参数 + bool
bool parse_int(const char* s, int* out); // 成功返回 true，结果写进 *out
// 问题：调用点啰嗦，out 可能忘了初始化，可能传 NULL

// 招式三：全局 errno / 特殊结构体
```

共同毛病：**「有没有值」和「值本身」混在一起**，靠文档约定，编译器不帮你检查。忘了判断就是 bug。

### 1.2 C++ 的答案：`std::optional<T>`

`std::optional<T>` 是一个「要么装着一个 T，要么是空」的盒子。类型上就写明了「这玩意可能没有」。

```cpp
#include <optional>
#include <string>

std::optional<int> parseInt(const std::string& s) {
    try {
        return std::stoi(s);      // 有值：直接返回，隐式包成 optional
    } catch (...) {
        return std::nullopt;      // 没值：返回空
    }
}

int main() {
    auto r = parseInt("42");
    if (r) {                      // 转 bool：有值为 true
        int v = *r;               // 解引用取值（像指针）
        // 或 r.value()
    }
    int x = parseInt("bad").value_or(-1);  // 没值就用默认值 -1
}
```

要点：
- `if (opt)` 判断有没有值；`*opt` / `opt.value()` 取值。
- `value_or(默认值)`：空时给个兜底，特别常用。
- `std::nullopt` 表示「空」。
- ⚠️ 对空的 optional 用 `*opt` 是**未定义行为**（和解空指针一样）；`opt.value()` 则会抛 `std::bad_optional_access`。先判断再取。

| | C 的做法 | `std::optional` |
|---|---|---|
| 表达"没有值" | 哨兵值 / 输出参数+bool | 类型自带，编译器可见 |
| 忘记判断 | 静默出 bug | 语义清晰，容易 review |
| 适用性 | 值域被占用就没法用 | 任何类型都能包一层 |

**什么时候用**：函数「正常情况下可能就是没有结果」——查表没查到、解析可能失败、可选配置项。**不是**用来报告错误（错误用异常或错误码，见第二部分）。

---

## 2. `std::variant` + `std::visit`：类型安全的联合体

### 2.1 C 的 union 有多危险

C 里想让一块内存「有时是 int，有时是 double，有时是字符串」，只能用 `union`：

```c
union Value { int i; double d; char* s; };
union Value v;
v.i = 42;
double bad = v.d;   // 灾难：你存的是 int，却按 double 读，垃圾数据，编译器不拦
```

`union` 的致命问题：**它不记得自己现在装的是哪个类型**。你得自己在旁边配一个 `enum tag` 手动维护，读的时候自己保证读对分支。错一次就是未定义行为。

### 2.2 C++ 的答案：`std::variant<Ts...>`

`std::variant` 是「记得住自己当前装的是哪个类型」的联合体，读错类型直接抛异常，不会静默出错。

```cpp
#include <variant>
#include <string>
#include <iostream>

std::variant<int, double, std::string> v;
v = 42;                      // 现在装 int
v = std::string("hello");    // 现在装 string

// 取值方式一：std::get<T>，类型不对会抛 std::bad_variant_access
std::string s = std::get<std::string>(v);

// 取值方式二：std::holds_alternative 先判断
if (std::holds_alternative<int>(v)) { /* ... */ }

// 取值方式三：std::get_if，返回指针，不匹配返回 nullptr
if (auto p = std::get_if<int>(&v)) { std::cout << *p; }
```

### 2.3 `std::visit`：按当前类型分派

最优雅的用法是 `std::visit` + 一个能处理所有类型的可调用对象。它保证你「每个可能的类型都处理到了」——漏一个编译不过。

```cpp
struct Printer {
    void operator()(int i)                { std::cout << "int: "    << i << "\n"; }
    void operator()(double d)             { std::cout << "double: " << d << "\n"; }
    void operator()(const std::string& s) { std::cout << "str: "    << s << "\n"; }
};

std::visit(Printer{}, v);   // 自动调用匹配当前类型的那个 operator()
```

> 进阶写法：用 lambda + `if constexpr`（见第 6 节）也能写 visitor，或者用「重载 lambda」技巧。教学阶段用 struct + 多个 `operator()` 最直白。

| | C 的 `union` | `std::variant` |
|---|---|---|
| 记录当前类型 | 手动维护 tag | 自动记录 |
| 读错类型 | 未定义行为，静默 | 抛异常，明确 |
| 能装非平凡类型（string） | 不能（要手动管理） | 能 |
| 穷尽处理 | 靠自觉 | `visit` 编译期检查 |

**什么时候用**：一个值「是有限几种类型之一」——比如解析结果是「数字 / 字符串 / 布尔」，或状态机的「几种状态各带不同数据」。比继承 + 虚函数更轻量（无堆分配、值语义）。

---

## 3. `std::string_view`：不拷贝的字符串视图

### 3.1 问题：为了「只读看一眼字符串」，却被迫拷贝

假设你写个函数统计字符串里的空格数，只读、不改。参数怎么传？

```cpp
int countSpaces(const std::string& s);  // 传 const std::string&
```

看着没问题，但如果调用方手里是个 `const char*`（C 字符串字面量），`countSpaces("hello world")` 会**构造一个临时的 std::string**——一次堆分配 + 拷贝，就为了看一眼。反过来，参数写成 `const char*` 又逼着 `std::string` 用户去 `.c_str()`。两边不讨好。

### 3.2 答案：`std::string_view`

`std::string_view` 就是「一个指针 + 一个长度」的轻量视图，不拥有、不拷贝底层字符。它能无缝接收 `std::string`、`const char*`、字符串字面量。

```cpp
#include <string_view>

int countSpaces(std::string_view sv) {   // 值传递，但只是 指针+长度，很廉价
    int n = 0;
    for (char c : sv) if (c == ' ') ++n;
    return n;
}

std::string str = "a b c";
countSpaces(str);          // 从 std::string 来，不拷贝
countSpaces("x y z");      // 从字面量来，不构造 string
countSpaces(str.substr(0, 3)); // 注意 substr 会拷贝；string_view 有自己的 substr 不拷贝
```

它还有 `substr()`、`find()` 等，且 `sv.substr(...)` **返回的还是视图，不拷贝**——处理大文本切片时性能优势明显。

### 3.3 悬垂风险（最重要的坑，必考）

`string_view` 不拥有数据，只是「指向别人的数据」。**如果被指向的数据先死了，视图就成了悬垂指针。**

```cpp
std::string_view dangling() {
    std::string local = "temp";
    return local;          // 灾难！返回后 local 析构，视图指向已释放内存
}                          // 用它就是未定义行为

std::string_view sv = std::string("temp");  // 同样灾难：临时 string 立刻析构
```

安全准则：
- `string_view` 适合当**函数参数**（调用期间实参一定活着）。
- **不要**把 `string_view` 存进成员变量或返回它，除非你能 100% 保证底层数据活得更久。
- **不要**用它指向临时对象。
- ⚠️ 另一个坑：`string_view` **不保证以 `\0` 结尾**，不能直接当 C 字符串传给 `printf("%s")` / `strlen`。要用先转 `std::string`。

| | `const char*` | `const std::string&` | `std::string_view` |
|---|---|---|---|
| 携带长度 | 否（要 strlen） | 是 | 是 |
| 接收字面量不拷贝 | 是 | 否（构造临时） | 是 |
| 接收 std::string | 要 .c_str() | 是 | 是 |
| 拥有数据 / 管生命周期 | 否 | 是 | 否（悬垂风险） |

**一句话**：只读字符串参数首选 `string_view`；但绝不持有它。

---

## 4. 结构化绑定：`auto [a, b] = ...`

C++17 让你「一次性把聚合体拆成几个命名变量」，代码立刻清爽。

### 4.1 拆 pair / tuple

```cpp
std::pair<int, std::string> getUser() { return {1, "Alice"}; }

auto [id, name] = getUser();   // id=1, name="Alice"，各自是独立命名变量
```

对比 C++17 之前：得先拿到 pair，再 `.first`/`.second`，或者用 `std::tie` + 预先声明变量，啰嗦。

### 4.2 遍历 map（最常用场景）

```cpp
#include <map>
std::map<std::string, int> scores{{"Alice", 90}, {"Bob", 85}};

for (const auto& [name, score] : scores) {   // 直接拆成 name / score
    std::cout << name << ": " << score << "\n";
}
```

比老写法 `it->first` / `it->second` 可读性高一个档次。

### 4.3 拆结构体

```cpp
struct Point { int x; int y; };
Point p{3, 4};
auto [px, py] = p;   // 按成员声明顺序绑定，px=3, py=4
```

要点：
- `auto [a, b]` 默认是拷贝；`auto& [a, b]` 绑引用（能改原对象，遍历大对象时避免拷贝）；`const auto& [a,b]` 只读不拷贝（遍历首选）。
- 绑定的数量必须和成员/元素个数完全一致，否则编译错误——这是好事，帮你发现结构变了。

---

## 5. `constexpr`：编译期计算，取代宏常量

### 5.1 C 的宏常量之痛

```c
#define MAX_SIZE 100          // 没类型、不进符号表、调试器看不到、能被 #undef
#define SQUARE(x) ((x)*(x))   // 函数式宏，重复求值、无类型检查（回顾 M6）
```

### 5.2 `constexpr` 变量与函数

`constexpr` 表示「这个值/这个函数调用能在**编译期**算出来」。它有类型、有作用域、能进调试器，还能当数组大小、模板参数。

```cpp
constexpr int kMaxSize = 100;        // 真正的编译期常量，有类型
int buffer[kMaxSize];                // 能当数组大小

constexpr int square(int x) {        // constexpr 函数
    return x * x;
}

constexpr int n = square(5);         // 编译期就算出 25，零运行时开销
int arr[square(3)];                  // 也能当数组大小
int runtime = 10;
int y = square(runtime);             // 传运行期值时，就退化成普通函数照常运行
```

要点：
- `constexpr` 函数**既能编译期算，也能运行期算**——看你传的实参是不是编译期常量。
- 它比宏安全：有类型检查、遵守作用域、无重复求值副作用。
- `const` 只表示「不可改」，值可能运行期才定；`constexpr` 更强，要求「编译期可求」。能用 `constexpr` 表达常量就别用宏。

| | `#define` 宏 | `constexpr` |
|---|---|---|
| 类型检查 | 无 | 有 |
| 作用域 | 无（全局污染） | 有 |
| 调试可见 | 否 | 是 |
| 能当数组大小/模板参数 | 是 | 是 |

---

## 6. `if constexpr`：编译期分支（简述）

普通 `if` 是运行期判断，两个分支都得能编译。`if constexpr` 是**编译期判断，条件为假的分支直接被丢弃、不参与编译**。主要用在模板里，根据类型走不同代码。

```cpp
template <typename T>
void printInfo(const T& val) {
    if constexpr (std::is_integral_v<T>) {   // 编译期就决定走哪条
        std::cout << "整数: " << val << "\n";
    } else {
        std::cout << "非整数: " << val << "\n";
    }
}
```

关键：因为假分支不编译，你可以在某分支写「只对某类型合法」的操作（比如只有指针类型才 `*val`），另一类型不会因此编译失败。这在 C++17 之前要靠 SFINAE 一堆模板黑魔法，现在一行 `if constexpr` 搞定。先知道它解决什么问题即可。

---

## 7. 枚举类 `enum class`：带作用域、强类型的枚举

### 7.1 C 的裸 enum 三宗罪

```c
enum Color { RED, GREEN, BLUE };
enum Fruit { APPLE, BANANA, RED_APPLE }; // 编译错误！RED... 其实不冲突，但下面这些是真坑

enum Color c = RED;
int x = RED;              // 罪一：隐式转成 int，RED 就是 0，容易误用
if (c == 0) {}            // 能过，但没意义
// 罪二：枚举名污染外层作用域，RED / GREEN 直接暴露在全局，容易撞名
// 罪三：底层类型不确定
```

### 7.2 `enum class`（强类型枚举）

```cpp
enum class Color { Red, Green, Blue };
enum class TrafficLight { Red, Green, Yellow };  // Red 不冲突，各有各的作用域

Color c = Color::Red;         // 必须带作用域 Color::
// int x = c;                 // 编译错误！不会隐式转 int（要转得 static_cast<int>(c)）
if (c == Color::Red) {}       // 只能和同类型比

enum class Status : uint8_t { Ok, Error };  // 还能指定底层类型，省内存/控制布局
```

好处全是「把 C 的坑堵上」：
- **不隐式转 int**，避免误用（要转得显式 `static_cast`）。
- **枚举名有作用域**（`Color::Red`），不污染、不撞名。
- **可指定底层类型**（`: uint8_t`）。

> 惯用法：新代码一律用 `enum class`。除非你确实需要隐式转整数的老式行为。

---

## 8. 属性（attributes）：`[[nodiscard]]`、`[[maybe_unused]]` 等

属性是 C++11 起标准化的「给编译器的提示」，写在 `[[ ]]` 里。几个常用的：

```cpp
[[nodiscard]] int compute();   // 调用者若忽略返回值，编译器警告
compute();                     // warning: 返回值被丢弃

[[nodiscard]] bool tryLock();  // 尤其适合"必须检查"的返回值（成功与否）

void f([[maybe_unused]] int debugFlag) {  // 告诉编译器"这参数可能没用到"，压掉未用告警
    // ...（release 版可能用不到 debugFlag）
}

switch (x) {
    case 1:
        doA();
        [[fallthrough]];       // 明确表示"我故意不 break，往下贯穿"，压掉告警
    case 2:
        doB();
        break;
}

[[deprecated("请改用 newApi()")]] void oldApi();  // 调用时警告，标记废弃
```

要点：
- `[[nodiscard]]` 最常用：标在「返回值不该被忽略」的函数/类型上（错误码、`optional`、锁的 `try_lock`）。开源库里到处是。
- `[[maybe_unused]]`、`[[fallthrough]]`：主要用来消除合理场景下的编译告警，让 `-Wall -Wextra` 干净。
- 属性不影响程序语义，只影响诊断（告警/错误）。忽略它们程序照样跑，但它们能帮你在编译期抓 bug。

---

## 第二部分：错误处理

## 9. 异常机制：`throw` / `try` / `catch`

### 9.1 C 只有错误码

C 报告错误只有一条路：**返回值 / 全局 errno**。

```c
FILE* fp = fopen("x.txt", "r");
if (!fp) { /* 每一层都得手动检查、手动向上传 */ return -1; }
```

痛点：错误处理和正常逻辑**交织在一起**；每一层都要检查、转发；深层出错要一路 `return -1` 传上来，中间任何一层忘了检查就丢了错误；构造函数根本没有返回值，没法报错。

### 9.2 C++ 的异常

异常把「出错」和「处理错」在代码上**分离**：出错的地方 `throw`，能处理的地方 `try/catch`，中间层什么都不用写。

```cpp
#include <stdexcept>

double divide(int a, int b) {
    if (b == 0)
        throw std::runtime_error("除数为零");  // 抛出，就地中断
    return static_cast<double>(a) / b;
}

int main() {
    try {
        double r = divide(10, 0);
        std::cout << r;                 // 不会执行
    } catch (const std::runtime_error& e) {   // 捕获（按引用！）
        std::cout << "出错: " << e.what() << "\n";
    } catch (const std::exception& e) {  // 基类兜底，能接住所有标准异常
        std::cout << "其他: " << e.what() << "\n";
    }
}
```

要点：
- `throw` 后，当前函数立刻中断，沿调用栈**向上找**能处理该类型的 `catch`（这个过程叫**栈展开**）。
- 标准异常都继承自 `std::exception`，`.what()` 给出描述。自定义异常也应继承它。
- **`catch` 一定按 `const 引用` 捕获**（`const std::exception&`），按值捕获会切片（回顾 M3 的对象切片）、还多一次拷贝。
- `catch (...)` 能接住一切（连非标准类型），但拿不到信息，慎用。
- 没被任何 `catch` 接住的异常 → 调用 `std::terminate` 直接终止程序。

### 9.3 `noexcept`：承诺「我不抛异常」

```cpp
void cleanup() noexcept {   // 承诺此函数不抛异常
    // 如果它真的抛了，程序直接 terminate，不会正常展开
}
```

意义：
- 是给编译器和调用者的**契约**，能启用优化。
- **移动构造/移动赋值应尽量标 `noexcept`**（回顾 M5）：`std::vector` 扩容时，只有元素的移动是 `noexcept`，它才敢用移动而非拷贝，否则为保证异常安全会退回拷贝。这是 `noexcept` 最实际的用途。
- 析构函数默认就是 `noexcept`——**永远不要让异常逃出析构函数**。

---

## 10. RAII 与异常安全：为什么栈展开时析构照常调用

这是理解异常安全的**钥匙**：栈展开时，已经构造好的局部对象会**按逆序自动析构**。这意味着——只要你用 RAII 管理资源（M4），异常发生时资源会自动释放，不会泄漏。

```cpp
void process() {
    std::lock_guard<std::mutex> lk(mtx);   // RAII 锁
    std::vector<int> data = load();        // RAII 容器
    if (data.empty())
        throw std::runtime_error("空数据"); // 抛异常！
    // ...
}   // 即使从 throw 处展开出去，lk 和 data 的析构也照常调用：锁被释放、内存被回收
```

对比 C：C 里 `goto cleanup` 手动清理，一旦某层忘了 `goto` 就泄漏；C++ 有异常 + RAII，清理是**自动且必然**的。

### 异常安全的三个级别（面试高频）

给一个操作，它对异常的保证分三档：

| 级别 | 承诺 | 说明 |
|---|---|---|
| **基本保证** | 不泄漏、对象仍有效 | 出异常后对象状态合法但可能已改变 |
| **强保证** | 要么成功，要么回到原状 | 事务性，失败等于没发生（如 `vector::push_back`） |
| **不抛保证** | 绝不抛异常 | `noexcept`，如析构、移动、swap |

实现强保证的常用手法是 **copy-and-swap**（M5 见过）：先在副本上操作，全成功了再 `swap`，swap 是 `noexcept` 的，所以要么整体成功要么原对象不动。

---

## 11. 异常 vs 错误码：何时用哪个

两者不是谁取代谁，是**分场景**：

| 维度 | 异常 | 错误码 / `optional` / `expected` |
|---|---|---|
| 适用 | 「异常」情况：罕见、破坏性、无法就地处理 | 「预期内」的失败：查不到、解析失败 |
| 正常路径开销 | 零开销（不抛就没成本） | 每次都要检查 |
| 抛出时开销 | 较大（栈展开） | 无 |
| 代码整洁度 | 正常逻辑不被打断 | 每层都要 if 检查转发 |
| 构造函数报错 | 只能用异常 | 构造函数没返回值，用不了错误码 |
| 跨模块/热路径 | 慎用（开销、二进制边界） | 首选 |

经验法则：
- **构造函数失败、深层不可恢复错误、违反前置条件** → 异常。
- **「没查到」「用户输入非法」这种预期内的普通结果** → `std::optional` 或错误码，别用异常（异常不是控制流）。
- 高性能热路径、和 C 交互的边界 → 错误码。

> C++23 有 `std::expected<T, E>`（类似 Rust 的 `Result`），C++17 还没有，但你可以用 `std::variant<T, Error>` 或 `std::optional` 顶一顶。

---

## 第三部分：并发基础

> 目标：建立正确心智模型，够用即止。并发的坑极深，这里教你「怎么安全地开线程、怎么保护共享数据」，不深入内存模型。

## 12. `std::thread`：创建、join、detach

C 里开线程要平台 API（pthread / Windows CreateThread）；C++11 起标准库统一提供 `std::thread`。

```cpp
#include <thread>
#include <iostream>

void work(int id) { std::cout << "线程 " << id << " 干活\n"; }

int main() {
    std::thread t(work, 1);   // 创建即启动，work(1) 在新线程跑
    // ... 主线程继续做别的 ...
    t.join();                 // 等 t 跑完（阻塞在这里直到结束）
    return 0;
}
```

要点（**必考坑**）：
- `std::thread` 对象析构前，**必须** `join()`（等它结束）或 `detach()`（放它自己跑，脱离管理）二选一。**两个都没调，析构时直接 `std::terminate`！**
- `join()`：阻塞当前线程直到目标线程完成。要拿结果、要保证顺序，用它。
- `detach()`：分离，让线程后台自生自灭。慎用——它可能访问已经销毁的变量。
- 传给线程的参数是**拷贝**进去的；要传引用得用 `std::ref`。

---

## 13. 数据竞争与 `std::mutex` + RAII 锁

### 13.1 数据竞争：多线程并发的头号 bug

**数据竞争**：两个及以上线程**同时**访问同一块内存，且至少一个在写，且没有同步。结果是**未定义行为**——不是「读到旧值」那么简单，是编译器/CPU 层面的彻底未定义。

```cpp
int counter = 0;
void inc() { for (int i = 0; i < 100000; ++i) ++counter; }  // 两个线程跑这个
// ++counter 不是原子的：读-改-写三步，两线程交错 -> 结果远小于预期，且每次不同
```

`++counter` 看着一行，实际是「读内存、加一、写回」三步。两个线程交错执行，就会丢更新。

### 13.2 `std::mutex` + `lock_guard`（RAII 锁）

用**互斥锁**保证「同一时刻只有一个线程」进入临界区。手动 `lock()`/`unlock()` 容易忘（尤其中途 return 或抛异常），所以用 **RAII 锁**：

```cpp
#include <mutex>

std::mutex mtx;
int counter = 0;

void inc() {
    for (int i = 0; i < 100000; ++i) {
        std::lock_guard<std::mutex> lk(mtx);  // 构造即 lock
        ++counter;                            // 临界区：受保护
    }                                         // lk 析构即 unlock（即使抛异常也解锁）
}
```

- `std::lock_guard`：最简单的 RAII 锁，构造上锁、析构解锁，不能中途手动解锁。首选。
- `std::unique_lock`：更灵活，能中途 `unlock()`/`lock()`、能延迟上锁、能转移所有权；配 `condition_variable` 时必须用它。代价是稍重。
- 核心思想和 M4 的 RAII 一模一样：**把「解锁」绑到对象析构上，永远不会忘、异常安全**。这就是面试问「什么是 RAII 锁」的答案。

---

## 14. `std::atomic`：无锁的原子操作入门

对于「就一个简单变量的计数、标志位」这种，上 mutex 太重。`std::atomic<T>` 让单个变量的操作变成**原子的**（不可分割，硬件保证），无需加锁。

```cpp
#include <atomic>

std::atomic<int> counter{0};

void inc() {
    for (int i = 0; i < 100000; ++i)
        ++counter;    // 原子自增，多线程安全，无数据竞争
}
```

`atomic` vs `mutex`（**高频面试题**）：

| | `std::atomic` | `std::mutex` |
|---|---|---|
| 保护对象 | 单个变量的单个操作 | 任意大小的临界区（多行、多变量） |
| 开销 | 小（硬件指令，无锁） | 大（可能阻塞、上下文切换） |
| 场景 | 计数器、标志位、简单状态 | 复合操作、保护数据结构 |
| 能保护"多步操作的一致性"吗 | 不能 | 能 |

一句话：**保护「一个变量的一次操作」用 `atomic`；保护「一段逻辑 / 多个变量的一致性」用 `mutex`。** 常见标志位 `std::atomic<bool> stop{false};` 用来通知线程退出。

---

## 15. `std::condition_variable`：线程间等待/通知（简述）

有时线程要「等某个条件成立再继续」（比如消费者等队列里有数据）。傻等（忙轮询）浪费 CPU。`condition_variable` 让线程**睡眠等待**，条件满足时被**唤醒**。

```cpp
std::mutex mtx;
std::condition_variable cv;
std::queue<int> q;

// 消费者
std::unique_lock<std::mutex> lk(mtx);
cv.wait(lk, [&]{ return !q.empty(); });  // 队列空就睡；被唤醒且条件真才继续
int item = q.front(); q.pop();

// 生产者
{
    std::lock_guard<std::mutex> lk(mtx);
    q.push(42);
}
cv.notify_one();   // 唤醒一个等待者
```

要点：
- `wait` 必须配 `unique_lock`（因为它要在等待时临时解锁、唤醒后重新上锁）。
- **一定用「带谓词的 wait」**（第二个参数是条件 lambda），它能自动处理**虚假唤醒**（线程可能无缘无故被唤醒）——没有谓词你就得手动 while 循环判断。
- 生产者-消费者模型是它的经典应用（本模块 mini 项目就是这个）。

---

## 第四部分：工程化

## 16. CMake 入门：让代码变成工程

单文件时 `cl file.cpp` / `g++ file.cpp` 就够了。项目一大（多文件、多库、跨平台），手敲编译命令就崩了。**CMake** 是事实标准的跨平台构建系统生成器：你写一份 `CMakeLists.txt` 描述「要构建什么」，CMake 生成对应平台的构建文件（VS 工程 / Makefile / Ninja）。

### 16.1 最小 `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.15)   # 要求的最低 CMake 版本
project(MyApp LANGUAGES CXX)            # 项目名 + 语言（CXX = C++）

set(CMAKE_CXX_STANDARD 17)              # 用 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)     # 强制，不允许降级
set(CMAKE_CXX_EXTENSIONS OFF)           # 关掉编译器私有扩展，用标准 C++

add_executable(myapp main.cpp)          # 定义可执行目标 myapp，由 main.cpp 构建
```

多文件只需把源文件都列上：

```cmake
add_executable(myapp main.cpp counter.cpp util.cpp)

# 或者先建一个库，再链接（工程里更常见）
add_library(core counter.cpp util.cpp)
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE core)
```

### 16.2 configure / build 流程

CMake 分两步，习惯「out-of-source build」（构建产物全放 `build/` 目录，不污染源码）：

```bash
cmake -S . -B build          # configure：读 CMakeLists.txt，在 build/ 生成构建文件
cmake --build build          # build：真正编译
```

- `-S .`：源码目录（含 CMakeLists.txt）。`-B build`：构建目录。
- 改了源码只需重跑 `cmake --build build`；改了 CMakeLists.txt，CMake 会自动重新 configure。

### 16.3 在 Visual Studio 里

现代 VS（2019+）**原生支持 CMake**：直接「打开文件夹」选中含 `CMakeLists.txt` 的目录，VS 会自动 configure，把 CMake 目标显示成可运行项，按 F5 就能构建调试。不需要手动建 `.sln`/`.vcxproj`。命令行也能用 `cmake -G "Visual Studio 17 2022"` 生成 VS 工程。

---

## 17. 单元测试：思路与零依赖极简断言

单元测试就是「写代码去验证代码」：给函数已知输入，断言输出符合预期。改代码后一跑，立刻知道有没有破坏原有功能（回归）。

### 17.1 零依赖极简断言

不引第三方库，一个宏 + `main` 就能起步：

```cpp
#include <iostream>
#include <cstdlib>

static int g_failures = 0;

#define CHECK(cond)                                            \
    do {                                                       \
        if (!(cond)) {                                         \
            std::cerr << "FAIL: " << #cond                     \
                      << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            ++g_failures;                                      \
        }                                                      \
    } while (0)

int add(int a, int b) { return a + b; }

int main() {
    CHECK(add(2, 3) == 5);
    CHECK(add(-1, 1) == 0);
    if (g_failures == 0) { std::cout << "全部通过\n"; return 0; }
    std::cerr << g_failures << " 个失败\n";
    return 1;   // 非零退出码 -> CI / CTest 判定为失败
}
```

关键：**测试失败要让程序返回非零退出码**，这样构建系统（CTest / CI）才能自动发现。`#cond` 是把条件表达式变成字符串，`__FILE__`/`__LINE__` 给出出错位置。

### 17.2 接入 CTest（可选）

CMake 自带测试驱动 CTest。把测试程序注册进去：

```cmake
enable_testing()
add_executable(tests tests.cpp)
add_test(NAME unit_tests COMMAND tests)   # 退出码非零即算失败
```

然后 `ctest --test-dir build` 一键跑全部测试。

### 17.3 用成熟框架（了解）

真实项目一般用 **GoogleTest** 或 **Catch2**，提供丰富断言（`EXPECT_EQ`、`ASSERT_THROW`）、自动发现、友好报告。CMake 里常用 `FetchContent` 自动拉取：

```cmake
include(FetchContent)
FetchContent_Declare(catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.5.0)
FetchContent_MakeAvailable(catch2)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
```

教学阶段先用零依赖断言建立「测试 = 断言 + 非零退出码」的心智，框架无非是把这套做得更漂亮。

---

## 18. 调试与排错工具

### 18.1 Sanitizer：抓内存/未定义行为的神器

Sanitizer 是编译期插桩的运行时检测器，跑一遍就能揪出肉眼极难发现的 bug。**开发/测试时开，release 不开**（有性能开销）。

- **AddressSanitizer (ASan)**：抓越界访问、use-after-free、内存泄漏、栈溢出。
- **UndefinedBehaviorSanitizer (UBSan)**：抓有符号溢出、空指针解引用、错误的类型转换等未定义行为。
- **ThreadSanitizer (TSan)**：抓数据竞争（并发 bug 神器，但和 ASan 不能同时开）。

开启方式：

```bash
# g++ / clang
g++ -std=c++17 -g -fsanitize=address,undefined main.cpp -o app
clang++ -std=c++17 -g -fsanitize=thread main.cpp -o app   # 查数据竞争

# MSVC（VS 2019 16.9+）
cl /EHsc /std:c++17 /fsanitize=address main.cpp
```

CMake 里通常加到编译/链接选项：

```cmake
target_compile_options(myapp PRIVATE -fsanitize=address,undefined)
target_link_options(myapp PRIVATE -fsanitize=address,undefined)
```

> 从 C 转过来的你，以前靠 `printf` 大海捞针查内存 bug；ASan 会直接告诉你「哪一行越界、这块内存是哪里分配/释放的」。养成开着 sanitizer 跑测试的习惯。

### 18.2 clang-tidy：静态分析与现代化建议

`clang-tidy` 不运行程序，只**读代码**就能指出问题：可疑写法、性能隐患、不符合现代 C++ 惯例的地方，还能自动修（比如把裸循环改成 range-for、建议用 `nullptr`）。

```bash
clang-tidy main.cpp -- -std=c++17
```

它和编译器告警互补：编译器管「语言规则」，clang-tidy 管「最佳实践」。CI 里常挂它做代码质量门禁。

---

## 19. 常见坑

1. **对空的 `optional` 直接解引用 `*opt`** → 未定义行为。先 `if (opt)` 或用 `value_or`。
2. **`std::variant` 用 `std::get<T>` 取错类型** → 抛 `bad_variant_access`。用 `holds_alternative` / `get_if` 先判断。
3. **`string_view` 指向临时对象或局部 string** → 悬垂，未定义行为。绝不持有、不返回、不指向临时。
4. **`string_view` 当 C 字符串用**（没有 `\0` 结尾）→ 传给 `printf("%s")`/`strlen` 出错。
5. **`std::thread` 析构前忘了 join/detach** → 直接 `terminate`。
6. **手动 `lock`/`unlock`，中途 return 或抛异常忘解锁** → 死锁/泄漏。永远用 `lock_guard`/`unique_lock`。
7. **多个锁加锁顺序不一致** → 死锁。约定「所有线程按固定顺序加锁」，或用 `std::scoped_lock`（C++17，一次锁多个、防死锁）。
8. **用 `atomic` 想保护「多步操作的一致性」** → 错，atomic 只保证单次操作原子。多步用 mutex。
9. **`condition_variable` 用不带谓词的 `wait`** → 被虚假唤醒坑。永远用带谓词的 `wait`。
10. **`catch` 按值捕获** → 对象切片 + 多余拷贝。永远 `catch (const std::exception&)`。
11. **异常逃出析构函数** → 栈展开期间再抛 → `terminate`。析构别抛。
12. **拿异常当普通控制流**（比如「没查到」也抛）→ 慢、乱。预期内的失败用 `optional`/错误码。

---

## 20. 高频面试点

- `std::optional` 解决什么问题？和「返回 -1」比好在哪？空时解引用会怎样？
- `std::variant` 和 C 的 `union` 区别？`std::visit` 干嘛的？
- `std::string_view` 是什么？什么时候能用、什么时候会悬垂？为什么不能持有它？
- 结构化绑定怎么遍历 map？`auto&` 和 `auto` 的区别？
- `constexpr` 和 `const`、和宏常量的区别？constexpr 函数一定编译期执行吗？
- `enum class` 相比裸 `enum` 的三个好处？
- `[[nodiscard]]` 什么场景用？
- 异常 vs 错误码怎么选？异常的开销在哪？
- 什么是异常安全？三个级别（基本/强/不抛）分别承诺什么？
- 为什么栈展开时析构照常调用？RAII 和异常安全什么关系？
- `noexcept` 有什么用？为什么移动构造要标 noexcept？
- `std::thread` 不 join 会怎样？join 和 detach 区别？
- 什么是数据竞争？什么是 RAII 锁？`lock_guard` 和 `unique_lock` 区别？
- `atomic` 和 `mutex` 什么区别？分别用在什么场景？
- 什么是死锁？怎么避免？
- `condition_variable` 为什么要用带谓词的 wait？什么是虚假唤醒？
- 为什么优先用 `std::make_unique`/`make_shared` 而不是 `new`？（回顾 M4：一次分配、异常安全、不写裸 new、`make_shared` 控制块和对象一次分配）

---

## 21. 编译提醒

单文件练习（x64 Native Tools 命令行）：
```
cl /EHsc /std:c++17 /W4 文件名.cpp
```
多文件：
```
cl /EHsc /std:c++17 /W4 main.cpp counter.cpp
```
用 CMake 的题目：
```
cmake -S . -B build
cmake --build build
```
开 sanitizer 排错（MSVC）：
```
cl /EHsc /std:c++17 /fsanitize=address 文件名.cpp
```
> 注：本机沙箱用 g++（ucrt64）验证语法：`g++ -std=c++17 -Wall -Wextra -fsyntax-only 文件.cpp`；线程相关加 `-pthread`。沙箱不能链接生成 exe，只做语法/编译检查。

---

至此 M1-M8 主线走完：从 C 到 C++（M1）、类与对象（M2）、继承多态（M3）、内存与 RAII（M4）、拷贝移动（M5）、模板（M6）、STL（M7）、现代特性与工程化（M8）。你现在具备读懂主流 C++ 代码库、上手真实工程的基础。下一步就是拿这套去写、去读别人的项目——比如你目标里的音视频方向，届时 `optional`/`variant`/RAII 锁/CMake 这些会天天见。

打开 `exercises.md` 开始练。

