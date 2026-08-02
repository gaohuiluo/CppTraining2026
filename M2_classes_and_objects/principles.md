# M2 类与对象（完整）

> 目标：把"类"彻底吃透。你在 C 里用 `struct` + 一堆操作它的函数来组织数据，C++ 的类就是把「数据 + 操作数据的函数 + 谁能访问」打包在一起。这一模块讲全：访问控制、构造/析构、初始化列表、`this`、`static`、友元、运算符重载。

---

## 0. 一句话总览

**类 = C 的 struct + 成员函数 + 访问控制 + 生命周期管理（构造/析构）。**
理解这一点，后面全是细节。

---

## 1. 从 C 的 struct 说起

C 里你大概这么写一个"栈"：

```c
// C 风格
typedef struct {
    int  data[100];
    int  top;
} Stack;

void stack_init(Stack* s)          { s->top = 0; }
void stack_push(Stack* s, int v)   { s->data[s->top++] = v; }
int  stack_pop(Stack* s)           { return s->data[--s->top]; }
```

问题：
- 数据和函数是**分离**的，靠命名约定（`stack_` 前缀）勉强关联。
- 谁都能直接改 `s->top`，没有保护，容易被外部破坏。
- 必须记得手动调 `stack_init`，忘了就是未定义行为。

C++ 的类把这些问题一次性解决：

```cpp
// C++ 风格
class Stack {
public:
    Stack() : top_(0) {}                    // 构造函数：自动初始化
    void push(int v) { data_[top_++] = v; }
    int  pop()       { return data_[--top_]; }
private:
    int data_[100];
    int top_;                               // 外部无法直接碰它
};

Stack s;          // 定义即自动初始化，不会忘
s.push(10);
```

对比逐条：
- 数据和函数**打包在一起**，`s.push(10)` 一眼看出是对 `s` 的操作。
- `private` 保护了 `top_`，外部改不了，只能通过 `push`/`pop`。
- 构造函数保证 `s` 一诞生就是合法状态。

---

## 2. `class` vs `struct`：只差一个默认

C++ 里 `struct` 也能有成员函数、构造函数、继承 —— 它其实就是"默认 public 的 class"。

| | 默认访问权限 | 默认继承方式 |
|---|---|---|
| `class` | `private` | `private` |
| `struct` | `public` | `public` |

除此之外**完全一样**。约定俗成：
- 只有数据、无行为的"聚合体" → 用 `struct`。
- 有封装、有行为的类型 → 用 `class`。

---

## 3. 访问控制：`public` / `private` / `protected`

- **`public`**：谁都能访问。对外的接口放这里。
- **`private`**：只有本类的成员函数（和友元）能访问。数据和内部实现放这里。
- **`protected`**：本类 + 派生类能访问（M3 继承时才有意义，先知道有这个）。

核心思想叫**封装（encapsulation）**：把数据设为 `private`，只暴露必要的 `public` 函数。好处是——外部代码依赖的是"接口"，你随时能改内部实现而不影响使用者。

```cpp
class Temperature {
public:
    void setCelsius(double c) {            // 提供受控的修改入口
        if (c < -273.15) c = -273.15;      // 可以在这里做校验！
        celsius_ = c;
    }
    double getCelsius() const { return celsius_; }
    double getFahrenheit() const { return celsius_ * 9.0 / 5.0 + 32.0; }
private:
    double celsius_ = 0.0;                 // 外部不能直接乱设成非法值
};
```

> 这就是"getter/setter"模式：数据私有，通过公共函数存取，存的时候能校验。C 的裸 struct 做不到。

---

## 4. 成员函数与 `const` 成员函数

成员函数就是"属于类的函数"。它天生能访问本类所有成员。

### `const` 成员函数（重点）
在函数后面加 `const`，表示**这个函数不会修改对象的成员**：

```cpp
double getCelsius() const { return celsius_; }
//                  ^^^^^ 承诺：我不改任何成员
```

规则：
- `const` 成员函数里，不能修改成员变量，也不能调用非 `const` 成员函数。
- **`const` 对象只能调用 `const` 成员函数。**

```cpp
const Temperature t;      // const 对象
t.getCelsius();           // OK：getCelsius 是 const 函数
t.setCelsius(20);         // 编译错误！setCelsius 会改成员，const 对象不许
```

> 惯用法：**任何"只读"的成员函数都加 `const`。** 这和 M1 的 const 正确性一脉相承，配合 `const T&` 传参才好用（回忆 M1 练习 4：`const std::string&` 传进来，只能调它的 const 函数）。

---

## 5. 构造函数（constructor）

构造函数在对象**创建时自动调用**，名字和类名相同，没有返回类型。作用：初始化对象。

### 5.1 几种构造函数

```cpp
class Point {
public:
    Point() {}                              // 默认构造(无参)
    Point(int x, int y) : x_(x), y_(y) {}   // 带参构造
private:
    int x_ = 0;
    int y_ = 0;
};

Point a;            // 调默认构造
Point b(3, 4);      // 调带参构造
Point c{3, 4};      // C++11 起也可用花括号(推荐，能防窄化)
```

### 5.2 初始化列表（member initializer list）—— 重点

注意 `Point(int x, int y) : x_(x), y_(y) {}` 里冒号后面那段，叫**初始化列表**。它在构造函数体执行**之前**，直接初始化成员。

对比两种写法：

```cpp
// 写法 A：初始化列表(推荐)
Point(int x, int y) : x_(x), y_(y) {}

// 写法 B：函数体内赋值
Point(int x, int y) { x_ = x; y_ = y; }
```

差别：
- 写法 B 是"先默认初始化成员，再赋值"，等于做了两次。
- 写法 A 是"直接用给定值初始化"，一步到位，更高效。
- **有些成员必须用初始化列表**：`const` 成员、引用成员、没有默认构造的类类型成员。它们没法"先默认再赋值"。

```cpp
class Widget {
public:
    Widget(int id) : id_(id) {}   // id_ 是 const，只能在初始化列表里给值
private:
    const int id_;
};
```

> 结论：**养成用初始化列表的习惯。**

⚠️ 一个坑：初始化列表的执行顺序**按成员声明顺序**，不是你写的顺序。所以别让一个成员的初始化依赖另一个后声明的成员。

### 5.3 默认参数简化重载
```cpp
class Point {
public:
    Point(int x = 0, int y = 0) : x_(x), y_(y) {}   // 一个顶三个
    // Point() / Point(3) / Point(3,4) 都能用
private:
    int x_, y_;
};
```

### 5.4 `explicit`（防止意外的隐式转换）
单参数构造函数默认允许隐式转换，有时会出意外：

```cpp
class MyString {
public:
    MyString(int size) { /* 分配 size 大小 */ }   // 本意：预留空间
};
void func(MyString s);
func(10);   // 竟然合法！10 被隐式转成 MyString(10)，多半不是你想要的
```

加 `explicit` 禁止这种隐式转换：

```cpp
explicit MyString(int size) { ... }
func(10);              // 现在编译报错
func(MyString(10));    // 必须显式写，意图清晰
```

> 惯用法：**单参数构造函数默认加 `explicit`**，除非你确实想要隐式转换。

---

## 6. 析构函数（destructor）—— C 完全没有的东西

析构函数在对象**销毁时自动调用**，名字是 `~类名`，无参无返回值。作用：清理对象占用的资源。

```cpp
class FileWrapper {
public:
    FileWrapper(const char* path) {
        fp_ = std::fopen(path, "r");
        std::cout << "打开文件\n";
    }
    ~FileWrapper() {                        // 析构：对象消失时自动调
        if (fp_) std::fclose(fp_);          // 有文件就关掉
        std::cout << "关闭文件\n";
    }
private:
    std::FILE* fp_ = nullptr;
};

void demo() {
    FileWrapper f("data.txt");   // 构造：打开文件
    // ... 用 f ...
}   // 函数结束，f 离开作用域 -> 自动调 ~FileWrapper() -> 关闭文件
```

关键点：
- 你**不用手动调**析构函数，对象离开作用域（或被 `delete`）时自动触发。
- 对比 C：C 里你得记得 `fclose`/`free`，忘了就泄漏。C++ 把"清理"绑到对象生命周期上。

> 这个"构造获取资源、析构释放资源"的模式，就是大名鼎鼎的 **RAII**，是 C++ 最重要的思想，M4 会专门深入。这里先建立直觉：**析构 = 自动清理。**

对象什么时候销毁：
- 局部对象：离开作用域时（如上例）。
- 成员对象：所属对象销毁时。
- `new` 出来的对象：`delete` 时（M4 细讲）。

---

## 7. `this` 指针

每个成员函数内部都有一个隐藏参数 `this`，指向"调用这个函数的对象"。

```cpp
class Counter {
public:
    void set(int value) {
        this->value_ = value;   // this-> 显式访问成员(通常可省略)
    }
    Counter& increment() {
        ++value_;
        return *this;           // 返回对象自身的引用 -> 支持链式调用
    }
    int get() const { return value_; }
private:
    int value_ = 0;
};

Counter c;
c.increment().increment().increment();   // 链式调用，得益于 return *this
std::cout << c.get();                     // 3
```

用途：
1. 成员名和参数名冲突时区分：`this->value_ = value;`
2. 返回 `*this` 实现链式调用（像 `std::cout << a << b`）。

---

## 8. `static` 成员

### 8.1 静态成员变量：属于类，不属于某个对象
所有对象**共享同一份**。

```cpp
class Widget {
public:
    Widget()  { ++count_; }
    ~Widget() { --count_; }
    static int count() { return count_; }   // 静态成员函数
private:
    static int count_;                       // 声明(类内)
};

int Widget::count_ = 0;                       // 定义(类外，必须有，C++17 前)

Widget a, b, c;
std::cout << Widget::count();   // 3，通过类名访问，不需要对象
```

对比 C：等价于 C 里的"全局变量 + 命名约定"，但 C++ 把它归到类的作用域里，更安全清晰。

> C++17 起可以用 `inline static int count_ = 0;` 在类内直接定义，省掉类外那行。

### 8.2 静态成员函数
- 没有 `this`（不针对某个对象）。
- 只能访问静态成员。
- 通过 `类名::函数()` 调用。

常见用途：计数、工厂函数、工具函数。

---

## 9. 友元（friend）

`private` 成员默认只有本类能碰。`friend` 声明可以**破例授权**某个外部函数或类访问私有成员。

```cpp
class Account {
public:
    Account(double balance) : balance_(balance) {}
    friend void audit(const Account& a);   // 授权 audit 函数访问私有成员
private:
    double balance_;
};

void audit(const Account& a) {
    std::cout << "余额: " << a.balance_ << "\n";   // 能访问 private！
}
```

要点：
- 友元**破坏封装**，是有意的例外，别滥用。
- 最常见的正当用途是**运算符重载**（下一节的 `operator<<`），因为它必须是非成员函数却要访问私有数据。
- 友元关系是单向的、不继承、不传递。

---

## 10. 运算符重载（operator overloading）

C++ 允许你为自定义类型定义 `+`、`==`、`<<` 等运算符的行为，让对象用起来像内置类型。

### 10.1 成员函数形式：`operator+`
```cpp
class Vec2 {
public:
    Vec2(double x = 0, double y = 0) : x_(x), y_(y) {}

    Vec2 operator+(const Vec2& rhs) const {     // a + b 会调 a.operator+(b)
        return Vec2(x_ + rhs.x_, y_ + rhs.y_);
    }
    bool operator==(const Vec2& rhs) const {
        return x_ == rhs.x_ && y_ == rhs.y_;
    }
    double x() const { return x_; }
    double y() const { return y_; }
private:
    double x_, y_;
};

Vec2 a(1, 2), b(3, 4);
Vec2 c = a + b;          // c = (4, 6)，等价于 a.operator+(b)
bool same = (a == b);    // false
```

### 10.2 非成员 + 友元形式：`operator<<`（让对象能被 cout 打印）
`std::cout << v` 里，左操作数是 `std::cout`（不是你的类），所以 `<<` 不能是你类的成员函数，得写成非成员函数，并声明为友元来访问私有成员：

```cpp
class Vec2 {
    // ... 同上 ...
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v);
};

std::ostream& operator<<(std::ostream& os, const Vec2& v) {
    os << "(" << v.x_ << ", " << v.y_ << ")";
    return os;              // 返回 os 以支持链式 << 
}

Vec2 a(1, 2);
std::cout << "a = " << a << "\n";   // a = (1, 2)
```

要点：
- 返回 `std::ostream&` 才能 `cout << a << b << c` 链下去。
- 参数用 `const Vec2&`（不拷贝、不修改）。

### 原则
运算符重载是"语法糖"，**只在语义自然时用**（数学向量加法、比较相等这种）。别为了炫技给不相关的类型乱定义 `+`，那会让代码更难懂。

---

## 11. 头文件与源文件拆分（工程习惯）

真实项目里，类的**声明**放头文件 `.h`，**实现**放源文件 `.cpp`。

```cpp
// Point.h ——声明
#pragma once                    // 防止头文件被重复包含(比 C 的 #ifndef 守卫简洁)
class Point {
public:
    Point(int x, int y);        // 只声明
    int sum() const;
private:
    int x_, y_;
};
```

```cpp
// Point.cpp ——实现
#include "Point.h"
Point::Point(int x, int y) : x_(x), y_(y) {}   // 类外定义用 类名::
int Point::sum() const { return x_ + y_; }
```

```cpp
// main.cpp
#include "Point.h"
int main() { Point p(3, 4); /* ... */ }
```

要点：
- `#pragma once` 放在头文件顶部，防重复包含（MSVC/主流编译器都支持）。
- 类外定义成员函数要写 `返回类型 类名::函数名(...)`。
- 练习阶段为了省事，我们仍多用单文件；但这个拆分方式你要会，M8 用 CMake 时是标配。

---

## 12. 常见坑

1. **忘了初始化列表里成员的初始化顺序按声明顺序**，不按你写的顺序。
2. **只读成员函数忘加 `const`**，导致 `const` 对象/`const T&` 参数没法调用它。
3. **单参数构造忘加 `explicit`**，引发意外隐式转换。
4. **在构造函数体里给 `const`/引用成员赋值** → 编译错误，它们必须在初始化列表里初始化。
5. **`operator<<` 写成成员函数** → 用不了 `cout << obj`，因为左操作数不是你的类。
6. **静态成员变量只声明没定义**（C++17 前）→ 链接错误 "unresolved external symbol"。
7. **析构函数里访问已经无效的资源**（比如 double free）→ 崩溃，注意置空判断。

---

## 13. 高频面试点（M2 相关）

- `class` 和 `struct` 的区别？（默认访问权限/继承方式）
- 构造函数初始化列表和函数体内赋值的区别？为什么推荐初始化列表？
- 哪些成员**必须**用初始化列表初始化？（const、引用、无默认构造的类成员）
- 成员初始化顺序由什么决定？（声明顺序）
- 析构函数的作用和调用时机？什么是 RAII？
- `const` 成员函数的含义？const 对象能调用非 const 成员函数吗？
- `explicit` 的作用？
- 静态成员变量存在哪、什么时候初始化？
- `this` 指针是什么？返回 `*this` 有什么用？
- 为什么 `operator<<` 通常是友元非成员函数？

---

## 14. 编译提醒

单文件练习照旧（x64 Native Tools 命令行）：
```
cl /EHsc /std:c++17 /W4 文件名.cpp
```
多文件（头文件+源文件）一起编译：
```
cl /EHsc /std:c++17 /W4 main.cpp Point.cpp
```

---

下一步：打开 `exercises.md`。这一模块的练习会让你反复写"完整的类"，把封装、构造/析构、运算符重载练成肌肉记忆。
