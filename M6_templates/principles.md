# M6 模板与泛型（完整）

> 目标：把"模板"这台泛型引擎彻底看懂。你在 C 里用 `#define` 宏和 `void*` 硬凑出来的"泛型"，C++ 用模板做得又安全又干净。这一模块讲全：函数模板、类模板、非类型参数、特化、默认参数、编译模型（为什么模板要放头文件），并教你看懂 STL 那一坨吓人的模板报错。STL 怎么用是 M7 的事，这里聚焦"模板机制本身"和"理解 STL 为什么长这样"。

---

## 0. 一句话总览

**模板 = 让编译器帮你按类型批量生成代码的"代码模具"。** 你写一份带类型占位符的蓝图，编译器在你用到某个具体类型时，才照着蓝图"填空"生成一份真正的代码。它是编译期的、类型安全的、零运行时开销的泛型。

记住这条主线，后面全是细节。

---

## 1. 从 C 的"泛型"说起：宏和 void*

C 没有泛型，但你总得写"对多种类型都能用"的代码。C 程序员有两招，两招都难受。

### 招式一：宏 `#define`

```c
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int   x = MAX(3, 5);        // ok
double d = MAX(1.5, 2.5);   // ok，看起来很泛型
```

宏的问题一箩筐：

- **没有类型检查**：宏只是文本替换，`MAX("abc", "xyz")` 照样"编译通过"，比较的却是指针地址，逻辑全错。
- **重复求值的副作用**：`MAX(i++, j)` 展开成 `((i++) > (j) ? (i++) : (j))`，`i++` 可能执行两次，行为诡异。
- **调试困难**：宏在预处理阶段就被展开没了，调试器里根本看不到 `MAX`，报错信息指向展开后的天书。
- **没有作用域**：宏无视命名空间和作用域，全局污染，撞名就出事。

### 招式二：`void*` 泛型

想写一个通用容器（比如通用链表、`qsort`），C 里只能靠 `void*` 抹掉类型：

```c
// C 标准库的 qsort，泛型全靠 void* + 一个比较回调
void qsort(void* base, size_t n, size_t size,
           int (*cmp)(const void*, const void*));
```

`void*` 的问题：

- **丢掉了类型信息**：编译器不知道 `void*` 指向什么，你传错类型它一声不吭，运行时才崩。
- **要手动传 size**：类型没了，元素大小得自己算、自己传，容易错。
- **到处强制转换**：用的时候满屏 `(int*)p`、`*(int*)a`，又丑又危险。
- **回调开销**：`qsort` 每次比较都走一次函数指针调用，编译器没法内联。

### C++ 模板：一份蓝图，编译器填空

同样的 `MAX`，模板版本：

```cpp
template <typename T>
T myMax(T a, T b) {
    return a > b ? a : b;
}

int    x = myMax(3, 5);        // 编译器生成 myMax<int>
double d = myMax(1.5, 2.5);    // 编译器生成 myMax<double>
```

逐条对比宏：

| | C 宏 `#define MAX` | C++ 函数模板 |
|---|---|---|
| 类型检查 | 无，文本替换 | 有，编译期严格检查 |
| 副作用 | 参数可能重复求值 | 是真函数，参数只求值一次 |
| 调试 | 展开后无踪迹 | 正常函数，能断点、能单步 |
| 作用域 | 无视作用域/命名空间 | 遵守作用域和命名空间 |
| 类型不匹配 | 悄悄出错 | 直接编译报错 |

对比 `void*`：模板生成的每份代码都带**完整类型信息**，不用传 size，不用强转，比较可以内联，**零运行时开销**。这就是"泛型"该有的样子。

> 一句话记牢：**宏是文本替换，模板是类型安全的代码生成。** 面试常问这个区别。

---

## 2. 函数模板

### 2.1 基本语法

```cpp
template <typename T>      // T 是类型占位符，随便起名，习惯用 T
T myMax(T a, T b) {
    return a > b ? a : b;
}
```

- `template <typename T>` 是模板头，声明后面这段代码里 `T` 是个"待定类型"。
- `typename` 也可以写成 `class`，二者在这里**完全等价**（`template <class T>` 一个意思）。习惯用 `typename`，语义更清楚。
- 这段代码本身**不是函数**，编译器不会为它生成任何机器码。只有当你用具体类型调用它时，才会"实例化"出真正的函数。

### 2.2 模板参数推导（argument deduction）

大多数时候你不用手写类型，编译器从实参**自动推导** `T`：

```cpp
myMax(3, 5);        // 实参是 int -> 推导 T = int
myMax(1.5, 2.5);    // 实参是 double -> 推导 T = double
```

推导有个坑：两个参数都是同一个 `T`，类型必须一致。

```cpp
myMax(3, 5.0);      // 编译错误！一个 int 一个 double，T 到底是啥？推导冲突
```

三种修法：

```cpp
myMax(3.0, 5.0);          // 1. 两边都给 double
myMax<double>(3, 5.0);    // 2. 显式指定 T = double，int 会转成 double
// 3. 改模板签名支持两个类型参数（见第 4 节）
```

### 2.3 显式指定模板参数

有时推导不出来，或你想强制某个类型，就用尖括号显式写：

```cpp
template <typename T>
T parseAs(const std::string& s) { /* ... */ }

// T 只出现在返回类型里，无法从参数推导，必须显式指定
auto v = parseAs<int>("42");
```

> 规律：**能从参数推导的就让编译器推；推不出来（比如 T 只在返回值里）才显式写。**

---

## 3. 类模板

### 3.1 基本语法：泛型 Stack

回忆 M2 那个只能装 `int` 的 `Stack`。用类模板，一份代码装任意类型：

```cpp
template <typename T>
class Stack {
public:
    void push(const T& v) { data_[top_++] = v; }
    T    pop()            { return data_[--top_]; }
    bool empty() const    { return top_ == 0; }
private:
    T   data_[100];
    int top_ = 0;
};
```

### 3.2 实例化时必须指定类型

和函数模板不同，**类模板不会自动推导**（C++17 有 CTAD 类模板参数推导，但入门先手写清楚）。用的时候尖括号里写明类型：

```cpp
Stack<int>         si;   // 一个装 int 的栈
Stack<std::string> ss;   // 一个装 string 的栈
si.push(10);
ss.push("hi");
```

`Stack<int>` 和 `Stack<std::string>` 是**两个完全不同的类型**，编译器为每个用到的类型各生成一份代码。

### 3.3 成员函数在类外定义的语法

如果把成员函数写在类外，语法会啰嗦一些——每个函数前都要带模板头，类名要带 `<T>`：

```cpp
template <typename T>
class Stack {
public:
    void push(const T& v);
    T    pop();
private:
    T   data_[100];
    int top_ = 0;
};

template <typename T>                 // 每个类外定义都要重复模板头
void Stack<T>::push(const T& v) {     // 类名是 Stack<T>，不是 Stack
    data_[top_++] = v;
}

template <typename T>
T Stack<T>::pop() {
    return data_[--top_];
}
```

> 注意：这些定义**也必须放在头文件里**，原因见第 8 节。这是从 C 过来最容易踩的坑。

---

## 4. 多个模板参数与非类型模板参数

### 4.1 多个类型参数

模板头里能放多个类型占位符，逗号分隔：

```cpp
template <typename T, typename U>
auto add(T a, U b) {          // 返回类型让 auto 自动推（见第 10 节）
    return a + b;
}

add(3, 5.0);    // T = int, U = double，合法
```

`std::pair<K, V>`、`std::map<K, V>` 就是这么来的——两个类型参数。

### 4.2 非类型模板参数（non-type template parameter）

模板参数不一定是类型，也可以是**编译期常量值**（整数、枚举、指针等）。最常见的是整数，用来做定长数组的大小：

```cpp
template <typename T, int N>       // T 是类型，N 是一个 int 值
class Array {
public:
    T&       operator[](int i)       { return data_[i]; }
    const T& operator[](int i) const { return data_[i]; }
    int      size() const            { return N; }
private:
    T data_[N];                    // N 在编译期就确定，栈上定长数组
};

Array<double, 16> a;   // 16 个 double 的定长数组
Array<int, 3>     b;   // 3 个 int
```

关键点：

- `N` 必须是**编译期常量**：`Array<int, 3>` 合法，`int n = 3; Array<int, n>` 不合法（`n` 不是编译期常量）。
- `Array<int, 3>` 和 `Array<int, 4>` 是**不同类型**，N 不同就是不同的类。
- 这正是标准库 `std::array<T, N>` 的原理——定长、栈上、零开销，和 C 的裸数组一样快但类型安全。

对比 C：C 里想要"元素类型和大小都可变"的数组容器，只能 `void*` + `malloc` + 手传 size。模板把这些搬到编译期，安全又高效。

---

## 5. 默认模板参数

和函数的默认参数一样，模板参数也能给默认值：

```cpp
template <typename T, int N = 8>       // N 默认 8
class Buffer {
    T data_[N];
};

Buffer<int>     b1;   // N = 8
Buffer<int, 32> b2;   // N = 32
```

类型参数也能有默认值。你在 STL 里见过一堆——`std::vector` 其实是 `std::vector<T, Allocator = std::allocator<T>>`，第二个参数默认给好了，所以你平时只写 `std::vector<int>`。

> 这就是为什么 STL 类型名一展开那么长：一堆你没写的默认模板参数都被编译器补上了。看懂这点，第 9 节的报错就没那么可怕。

---

## 6. 模板特化

有时"通用蓝图"对某个特定类型不够好，你想给它开个小灶——这就是**特化（specialization）**。

### 6.1 全特化（full specialization）

为某个**具体类型**单独写一份实现。经典例子：通用的比较对 `const char*` 是错的（比的是指针不是内容），给它全特化：

```cpp
template <typename T>
bool equal(T a, T b) {           // 通用版本
    return a == b;
}

template <>                       // 全特化：模板头空着
bool equal<const char*>(const char* a, const char* b) {
    return std::strcmp(a, b) == 0;   // 比字符串内容，不是比指针
}

equal(1, 1);              // 用通用版
equal("hi", "hi");        // 用 const char* 特化版
```

`template <>` 空模板头 + 类名/函数名后跟具体类型，就是全特化。类模板同理：

```cpp
template <typename T>
class TypeName {
public:
    static const char* get() { return "unknown"; }
};

template <>                       // 为 int 全特化
class TypeName<int> {
public:
    static const char* get() { return "int"; }
};
```

### 6.2 偏特化（partial specialization，入门够用即可）

**只针对类模板**（函数模板不支持偏特化）。它不指定到某个具体类型，而是限定成"某一类"类型，比如"所有指针类型"：

```cpp
template <typename T>
class Holder {                    // 通用版
public:
    static const char* kind() { return "value"; }
};

template <typename T>
class Holder<T*> {                // 偏特化：匹配任意指针类型 T*
public:
    static const char* kind() { return "pointer"; }
};

Holder<int>   a;   // 用通用版 -> "value"
Holder<int*>  b;   // 用偏特化 -> "pointer"
Holder<char*> c;   // 也用偏特化 -> "pointer"
```

偏特化的模板头**不为空**（`template <typename T>`），但类名后跟一个"模式"（`Holder<T*>`）。它是标准库很多"类型萃取（type traits）"的实现基础，入门知道有这么回事、能看懂就够了，M7 及以后会用到。

---

## 7. 实例化：模板是"惰性"的

理解模板报错和编译模型，先建立一个核心直觉：**模板本身不生成代码，只有被用到的具体类型才触发"实例化"（instantiation），生成真正的代码。**

```cpp
template <typename T>
T myMax(T a, T b) { return a > b ? a : b; }

// 到此为止，编译器一行机器码都没生成

int x = myMax(3, 5);   // 这里！编译器才实例化出 myMax<int>
```

两个推论，都很重要：

1. **没用到就不实例化**：如果你从没用 `myMax<double>`，编译器就不生成它。这也是为什么模板代码就算某个类型用起来会出错，只要你没实例化那个类型，就不报错。
2. **实例化时才检查类型是否"支持"模板里的操作**：`myMax` 里用了 `>`，如果你拿一个没重载 `>` 的类型去实例化，**报错发生在实例化那一刻**，而且错误信息往往指向模板内部。这是模板报错冗长的根源（第 9 节）。

对比 C：C 的函数在定义时就编译成一份固定的机器码。模板不是——它是"按需生成"，用几个类型就生成几份。这也是模板代码可能让**编译变慢、二进制变大**（代码膨胀）的原因。

---

## 8. 模板与编译模型：为什么模板要放头文件

这是从 C 过来**最容易踩的坑**，单独拎出来讲透。

### 8.1 先回忆普通函数：声明/定义分离

C 和 C++ 里普通函数可以声明放头文件、定义放 `.cpp`：

```cpp
// util.h
int add(int a, int b);          // 只有声明

// util.cpp
int add(int a, int b) { return a + b; }   // 定义

// main.cpp
#include "util.h"
int r = add(1, 2);              // 编译 main.cpp 时只需要声明就够了
```

为什么行得通？因为 `add` 是个具体函数，编译 `main.cpp` 时编译器只要知道"有这么个函数、签名长这样"就能生成调用代码，`add` 的实体由 `util.cpp` 单独编译出来，最后**链接器**把两边拼起来。

### 8.2 模板为什么不行

模板不是具体代码，是"生成代码的蓝图"。编译器要实例化 `myMax<int>`，**必须看到 `myMax` 的完整定义**（函数体），光有声明没法填空。

假如你学 C 的习惯，把模板定义放进 `.cpp`：

```cpp
// mymax.h
template <typename T> T myMax(T a, T b);      // 只有声明

// mymax.cpp
template <typename T> T myMax(T a, T b) { return a > b ? a : b; }  // 定义

// main.cpp
#include "mymax.h"
int x = myMax(3, 5);   // 编译 main.cpp 时看不到定义，无法实例化 myMax<int>
```

编译 `main.cpp` 时，编译器只看到声明，生成不了 `myMax<int>`。编译 `mymax.cpp` 时，编译器有定义，但**没人用到任何类型**，所以它也不实例化。结果：`myMax<int>` 谁都没生成，链接时报错：

```
undefined reference to `int myMax<int>(int, int)'
```

### 8.3 正确做法：声明和定义都放头文件

```cpp
// mymax.h
#pragma once
template <typename T>
T myMax(T a, T b) {          // 定义直接写在头文件里
    return a > b ? a : b;
}
```

这样每个 `#include "mymax.h"` 的 `.cpp` 都能看到完整定义，用到什么类型就地实例化。

> 结论：**模板（函数模板、类模板的成员函数）的定义要和声明一起放在头文件里。** 类模板即使成员函数写在类外，也要跟类定义待在同一个头文件。这就是为什么 STL 全是头文件（`<vector>`、`<map>` 里全是模板定义），几乎没有对应的 `.cpp`。

（补充：真要分离也有 `.tpp`/`.ipp` 约定或"显式实例化"的手段，但入门阶段记住"模板放头文件"这条铁律就够了。）

---

## 9. 看懂 STL 的模板报错

模板报错以"又长又吓人"著称。别慌，掌握定位方法，其实一两行就能找到真正的问题。

### 9.1 为什么报错这么长

前面说过：模板出错发生在**实例化那一刻**，而且错误往往在模板内部（STL 源码深处）。编译器会把整条"实例化调用链"全打出来，加上被默认模板参数展开的超长类型名（`std::vector<int, std::allocator<int>>` 这种），一屏都装不下。

### 9.2 典型例子一：类型不支持某操作

```cpp
#include <algorithm>
#include <vector>
struct P { int x; };            // 没有重载 operator<
int main() {
    std::vector<P> v(3);
    std::sort(v.begin(), v.end());   // sort 内部要用 < 比较元素
}
```

报错会有几十行，核心那句长这样（g++ 简化后）：

```
error: no match for 'operator<' (operand types are 'P' and 'P')
   ... in instantiation of ... std::sort<...> ...
```

定位方法：

- **从下往上、从你自己的代码找起**。报错里会有一行 `required from here` 或 `in instantiation of`，指向**你写的那行**（`std::sort(...)`）——那才是导火索。
- **抓 `error:` 那一行的核心信息**，忽略中间一大堆 `std::__` 内部类型。这里核心是 `no match for 'operator<'`，翻译过来：`P` 没有 `<`，而 `sort` 需要它。
- 修复：给 `P` 加 `operator<`，或给 `sort` 传自定义比较器。

### 9.3 典型例子二：const 用错 / 类型不匹配

```cpp
#include <vector>
int main() {
    std::vector<int> v = {1, 2, 3};
    std::vector<std::string> s = v;   // 想用 int 的 vector 初始化 string 的 vector
}
```

核心报错：

```
error: conversion from 'std::vector<int>' to non-scalar type
       'std::vector<std::string>' requested
```

这个反而清楚：两个 `vector` 元素类型不同，不能直接拷贝。

### 9.4 看报错的通用套路

1. **只看第一个 `error:`**：后面的错误经常是第一个引发的连锁反应，修好第一个再说。
2. **找 `required from` / `in instantiation of ... here`**：顺着它找到**你自己代码**里那个触发行。
3. **在脑子里把长类型名"折叠"**：`std::vector<int, std::allocator<int>>` 就当 `vector<int>` 看，`std::__cxx11::basic_string<char, ...>` 就当 `string` 看。
4. **抓关键词**：`no match for` / `no member named` / `incomplete type` / `static assertion failed` —— 这些词直接告诉你缺了什么。

> 经验：90% 的 STL 模板报错，根因就是"你给的类型不满足这个模板/算法的要求"（缺 `<`、缺默认构造、缺某个成员）。练几次就有肌肉记忆了。

---

## 10. auto / decltype 与模板的关系（简述）

`auto` 和模板参数推导是**同一套规则**。你可以把 `auto` 理解成"匿名的模板参数推导"：

```cpp
auto x = 3 + 5;         // auto 推导成 int，规则和 template<typename T> f(T x) 推 T 一样
```

在模板里，有时返回类型依赖参数类型，写不出来，就用 `auto` 或 `decltype` 让编译器算：

```cpp
template <typename T, typename U>
auto add(T a, U b) {           // C++14 起：返回类型让编译器从 return 推
    return a + b;
}

// decltype：拿到某个表达式的类型（不求值）
template <typename T, typename U>
auto add2(T a, U b) -> decltype(a + b) {   // 尾置返回类型，显式写出"a+b 的类型"
    return a + b;
}
```

- `auto` 返回：编译器看 `return` 语句推导，简单够用。
- `decltype(表达式)`：得到表达式的类型，常配合尾置返回类型 `-> decltype(...)`，在 C++11 时代很常见。
- 入门知道"模板里返回类型算不出来时，用 `auto`/`decltype` 兜底"就行，深入的完美转发、`decltype(auto)` 以后再说。

---

## 11. 变参模板（简单提及）

模板参数个数也能不定，叫**变参模板（variadic template）**，用 `...` 表示"一包类型/参数"：

```cpp
template <typename... Args>          // Args 是"一包"类型
void print(Args... args) {           // args 是"一包"参数
    // C++17 折叠表达式(fold expression)：把 << 折叠到整包参数上
    (std::cout << ... << args) << "\n";
}

print(1, "hello", 3.14);   // 一次传任意个、任意类型的参数
```

- `typename... Args` 叫**模板参数包**，`Args... args` 叫**函数参数包**。
- `(std::cout << ... << args)` 是 **C++17 折叠表达式**，一行把整包参数依次 `<<` 出去，取代了 C++11 那种递归展开的繁琐写法。
- 这就是 `printf` 的类型安全升级版——`std::printf` 靠 `...` 变参和格式串（不安全），变参模板全程有类型检查。

入门点到为止：知道 STL 里 `std::make_unique`、`emplace_back` 这类"转发任意参数"的函数是靠变参模板实现的，能看懂 `typename... Args` 就够了。

---

## 12. 常见坑

1. **模板定义放进 `.cpp`** → 链接错误 `undefined reference to ...`。模板定义必须放头文件（第 8 节，头号大坑）。
2. **以为模板会像普通函数一样"提前编译"**：模板是惰性实例化，用到才生成，没用到的类型永远不检查。
3. **类模板忘了写类型参数**：`Stack s;` 错，必须 `Stack<int> s;`（类模板不自动推导，除非 C++17 CTAD）。
4. **函数模板推导冲突**：`myMax(3, 5.0)` 里 int/double 让同一个 `T` 推导失败，要么统一类型，要么显式 `myMax<double>(...)`，要么用两个类型参数。
5. **非类型参数传了非编译期常量**：`int n = 5; Array<int, n> a;` 错，`N` 必须是编译期常量。
6. **函数模板想偏特化**：不支持。函数模板只能全特化或用重载替代；偏特化只对类模板。
7. **`Stack<int>` 和 `Stack<double>` 当成同一类型**：它们是彻底不同的类型，不能互相赋值、不能放进同一个非模板容器。
8. **代码膨胀无感知**：用几十个不同类型实例化同一个大模板，二进制会明显变大、编译变慢。入门不用优化，但要知道有这回事。

---

## 13. 高频面试点（M6 相关）

- **模板和宏（`#define`）的区别？** 宏是预处理阶段的文本替换，无类型检查、有重复求值副作用、无作用域、难调试；模板是编译期类型安全的代码生成，是真函数/真类型。
- **模板和 `void*` 泛型的区别？** `void*` 丢失类型、要手传 size、满屏强转、有回调开销；模板保留完整类型信息、零运行时开销、类型安全。
- **为什么模板定义要放在头文件里？** 编译器实例化时必须看到完整定义才能"填空生成代码"，声明放头文件、定义放 `.cpp` 会导致实例化时看不到定义，链接报 `undefined reference`。
- **模板什么时候实例化？** 惰性——只有被某个具体类型用到时才实例化生成代码，没用到的类型不生成、不检查。
- **`typename` 和 `class` 在模板参数里有区别吗？** 声明模板类型参数时完全等价（`template <typename T>` == `template <class T>`）。（`typename` 另有"指明依赖类型"的用途，进阶再说。）
- **什么是全特化、偏特化？函数模板能偏特化吗？** 全特化针对具体类型，偏特化针对"一类"类型（如所有指针）；函数模板只能全特化，偏特化只对类模板。
- **非类型模板参数是什么？举例？** 编译期常量做模板参数，如 `std::array<T, N>` 的 `N`。
- **模板会带来什么代价？** 编译变慢、二进制膨胀（每个用到的类型各生成一份代码）。
- **`auto` 的类型推导和模板推导什么关系？** 同一套规则。

---

## 14. 编译提醒

单文件练习（x64 Native Tools 命令行）：
```
cl /EHsc /std:c++17 /W4 文件名.cpp
```
多文件（头文件 + 源文件）一起编译：
```
cl /EHsc /std:c++17 /W4 main.cpp other.cpp
```
> 注意：模板全在头文件里，头文件不参与单独编译，命令行里只列 `.cpp`。本模块大多是单文件，头文件+源文件的那题按上面第二行来。

---

下一步：打开 `exercises.md`，把函数模板、类模板、非类型参数、特化都写一遍，最后用一个泛型 `Stack<T>` / `Array<T, N>` 把本模块串起来。

写完模板机制，M7 会带你**用** STL——`vector`、`map`、`string`、算法、迭代器。到时候你会发现：你在 M6 手写的 `Stack<T>`、`Array<T, N>`，正是 STL 那些容器的"简化原型"。理解了模板，STL 就不再是黑盒。


