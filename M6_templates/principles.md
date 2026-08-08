# M6 模板与泛型：编译期的代码生成器

> 目标：这一模块只讲透一件事——**模板到底是什么、编译器拿它做了什么**。你在 C 里用 `#define` 宏和 `void*` 硬凑泛型的痛都尝过，C++ 的答案是模板。但模板的语法规则又多又怪：为什么定义必须放头文件？为什么报错动辄几十行？为什么 `Stack<int>` 和 `Stack<double>` 不能互相赋值？这些规则如果一条条死记，M6 就是一堆散沙。好消息是：**它们全是同一个本质的推论**。抓住那个本质，所有规则自己长出来。STL 怎么用是 M7 的事，这里聚焦机制本身。

---

## 0. 一句话总览与全文主线

**模板不是函数、不是类，是一份交给编译器的「代码配方」；编译器在你用到某个具体类型的那一刻，才照配方生成一份该类型专属的真代码。** 模板 = 编译期的代码生成器。

整篇文章走这条主线，先画在这里：

```
泛型的本质需求：一份逻辑，多种类型
        │
        ├─ C 方案一：宏 ──── 文本替换，无类型检查、重复求值    （残缺）
        ├─ C 方案二：void* ── 抹掉类型，运行期开销、满屏强转    （残缺）
        │
        └─ C++ 方案：模板 = 编译期代码生成器
                │
                │  由「编译期按需生成代码」这一个本质，推出全部规则：
                │
                ├─ 生成时要看到配方全文 ──→ 模板定义必须放头文件（否则 LNK2019）
                ├─ 用到才生成           ──→ 惰性实例化（没用到的成员不检查）
                ├─ 每个类型各生成一份    ──→ Stack<int>≠Stack<double>、代码膨胀
                └─ 生成物在使用点才编译  ──→ 类型检查推迟到实例化点、报错又长又深
```

后面每一节都是在这张图上填肉。读完你应该能做到：别人问任何一条模板规则的「为什么」，你都能从「编译期代码生成」这一个源头把它推出来。

---

## 1. 泛型的本质需求，和 C 的两个残缺方案

### 1.1 需求从哪来

你在 C 里一定写过这种代码：`maxInt(int, int)`、`maxDouble(double, double)`、`maxFloat(float, float)`——**逻辑一模一样，只有类型不同**。逻辑只有一份，代码却要抄 N 份，改一处 bug 要改 N 处。「一份逻辑、多种类型」就是泛型（generic programming）要解决的问题。

C 没有泛型语法，但需求躲不掉，于是 C 程序员发明了两个变通方案。两个你都用过，两个的痛你也都懂。

### 1.2 方案一：宏——把「生成代码」交给预处理器

```c
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int    x = MAX(3, 5);        // 展开成 ((3) > (5) ? (3) : (5))
double d = MAX(1.5, 2.5);    // 看起来很泛型
```

宏的思路其实摸到了正确方向的边——**它也是「生成代码」**，只不过生成这件事交给了预处理器，而预处理器只会干一件事：无脑文本替换。它不懂类型、不懂语法、不懂作用域，于是：

- **没有类型检查**：`MAX("abc", "xyz")` 照样通过，比较的却是两个指针的地址，逻辑全错，编译器一声不吭。
- **重复求值**：`MAX(i++, j)` 展开成 `((i++) > (j) ? (i++) : (j))`，`i++` 可能执行两次。这不是 bug，是文本替换的必然结果——文本里 `a` 出现两次，替换后 `i++` 就出现两次。
- **调试失踪**：预处理跑完宏就没了，调试器里看不到 `MAX`，报错指向展开后的天书。
- **无作用域**：宏无视命名空间，全局污染，撞名就出事。

### 1.3 方案二：`void*`——把类型抹掉换通用

写通用容器和通用算法时宏不够用了（宏没法生成一个「结构体」），C 的第二招是用 `void*` 抹掉类型。标准库的 `qsort` 就是代表作：

```c
void qsort(void* base, size_t n, size_t size,
           int (*cmp)(const void*, const void*));
```

宏的问题是「生成代码但不懂类型」，`void*` 反过来：**它根本不生成新代码，而是让一份代码通过丢弃类型信息来通吃所有类型**。代价：

- **类型信息没了**：编译器不知道 `void*` 后面是什么，传错类型不报错，运行时才崩。
- **size 要手传**：类型没了，元素多大编译器不知道，`sizeof` 得你自己算自己传，传错就是内存错误。
- **满屏强转**：用的时候 `*(const int*)a`，又丑又危险。
- **运行期开销**：`qsort` 每次比较走一次函数指针，编译器无法内联；而你手写的 `sortInt` 里 `a < b` 是一条机器指令。**通用性是拿性能换的。**

### 1.4 两个方案的病根，和 C++ 的药方

把两个方案放一起看，病根就清楚了：

| | 宏 | `void*` | 理想方案 |
|---|---|---|---|
| 思路 | 编译前生成代码 | 一份代码丢类型通吃 | **编译期生成代码** |
| 谁来做 | 预处理器（不懂类型） | 程序员（手动管理类型） | **编译器（最懂类型）** |
| 类型检查 | 无 | 无 | 有，编译期 |
| 运行期开销 | 无 | 有（函数指针、不能内联） | 无 |
| 类型信息 | 不感知 | 主动丢弃 | 完整保留 |

宏方向对（生成代码）但执行者不行（预处理器不懂类型）；`void*` 执行者勉强（程序员懂类型）但方向不行（丢类型换通用）。C++ 的答案是把两者的优点拼起来：**还是生成代码，但把生成这件事交给全工程里最懂类型的那个角色——编译器**。这就是模板。

```cpp
template <typename T>
T myMax(T a, T b) {
    return a > b ? a : b;
}

int    x = myMax(3, 5);        // 编译器生成一份 myMax<int>
double d = myMax(1.5, 2.5);    // 编译器生成一份 myMax<double>
```

生成出来的 `myMax<int>` 是一个**如假包换的普通函数**：有类型检查、参数只求值一次、能打断点、遵守作用域、比较可内联、零运行期开销。宏和 `void*` 的每一条病，它都没有。

> 一句话记牢：**宏是预处理期的文本替换，`void*` 是运行期的类型擦除，模板是编译期的类型安全代码生成。** 面试必问，1.4 的表就是答案。

---

## 2. 模板不是函数，是「配方」——用编译器的眼睛看一遍

这是全文最重要的一节。想彻底搞懂模板，你需要换一双眼睛：不做「写代码的人」，做一次编译器。

### 2.1 先纠正一个直觉：这段代码不是函数

```cpp
template <typename T>
T myMax(T a, T b) {
    return a > b ? a : b;
}
```

从 C 过来，你的直觉是「这是个函数定义，编译器读到它就编译出一份机器码」。**错了，这是模板最反直觉、也最要紧的一点：这段代码不产生任何机器码。** 它是一份配方（蓝图），`T` 是配方里的空槽。配方本身不能吃，照配方做出来的菜才能吃；模板本身不能调用，用它生成出来的函数才能调用。

`template <typename T>` 是模板头，声明「下面这段代码里 `T` 是待定类型」。`typename` 写成 `class` 完全等价（`template <class T>` 是老写法），习惯用 `typename`，语义更准。

### 2.2 编译器视角：`myMax(3, 5)` 这一行发生了什么

现在你是编译器，正在逐行编译 `main.cpp`，读到了 `int x = myMax(3, 5);`。你依次做五件事：

```
第 1 步【查名字】  myMax 是什么？查到了：一个函数模板（一份配方，不是函数）。

第 2 步【推导】    配方要填 T。看实参：3 和 5 都是 int → T = int。

第 3 步【查缓存】  本编译单元之前生成过 myMax<int> 吗？
                  有 → 直接用，跳到完；没有 → 继续。

第 4 步【生成】    把 T = int 代进配方，"抄"出一份真函数：
                      int myMax(int a, int b) { return a > b ? a : b; }
                  ★ 这一步要求我手里有配方全文（函数体）。
                    只有一行声明，我抄什么？

第 5 步【编译生成物】现在才对这份生成物做完整的类型检查：
                  int 支持 operator> 吗？支持。→ 编译成机器码，收工。
```

第 4 步叫**实例化（instantiation）**，`myMax<int>` 叫模板的一个**实例**。写 `myMax<double>(1.0, 2.0)` 就再走一遍流程，生成第二份独立的函数。你也可以跳过第 2 步直接点菜：`myMax<double>(3, 5.0)` 显式指定 `T = double`（第 3 节细说）。

### 2.3 本质定了，规则全是推论

盯着上面五步看，模板的四条「怪规则」全在里面了：

| 五步中的哪一步 | 推论 | 表现出来的规则/现象 | 详见 |
|---|---|---|---|
| 第 4 步要看到配方全文 | 定义必须对使用点可见 | **模板定义必须放头文件**，放 `.cpp` 就 LNK2019 | 第 6 节 |
| 第 3、4 步：用到才生成 | **惰性实例化** | 没用到的实例不生成、不检查；类模板的成员函数一个一个单独惰性 | 第 7 节 |
| 每种 T 各走一遍第 4 步 | 每个类型一份独立代码 | `Stack<int>` 和 `Stack<double>` 是两个类型；实例多了**代码膨胀、编译变慢** | 第 4、5 节 |
| 第 5 步才做类型检查 | 检查推迟到实例化点 | 报错发生在「用」的那一刻，错误指向模板内部，带一条实例化链，**又长又深** | 第 9 节 |

这张表是全文的骨架。接下来先把语法层面（函数模板、类模板、非类型参数）过扎实，然后逐条把推论讲透。

---

## 3. 函数模板：推导、显式指定、多参数

### 3.1 实参推导：编译器怎么猜出 T

大多数时候你不写 `<int>`，编译器从实参**推导（deduce）**出 `T`。推导规则完整版能写一本书（《Effective Modern C++》开篇就是它），但日常 95% 的场景只需要三条。每条都给你一个 `static_assert` 例子——**它们能直接编译验证，编译通过即证明规则如所说**（`static_assert` 是编译期断言，条件为假直接编译失败）。

先备好两个探针模板：

```cpp
#include <type_traits>

template <typename T> T  probeVal(T x);    // 模拟按值传参的模板 f(T x)
template <typename T> T& probeRef(T& x);   // 模拟按引用传参的模板 f(T& x)
```

（只有声明没有定义也行——`decltype` 不真调用函数，只问类型。）

**规则 1：按值传参（`T x`），实参的引用和顶层 const 都被剥掉。**

```cpp
int        i   = 0;
const int  ci  = 0;
const int& cri = i;

static_assert(std::is_same_v<decltype(probeVal(i)),   int>);
static_assert(std::is_same_v<decltype(probeVal(ci)),  int>);  // const 被剥掉
static_assert(std::is_same_v<decltype(probeVal(cri)), int>);  // 引用和 const 都剥掉
```

为什么剥？想通了就不用背：按值传参，形参是实参的**一份拷贝**。原物是不是 const、是不是引用，管不到拷贝头上——你把一份 `const int` 复制给我，我这份副本凭什么不能改？所以推导时这些属性直接丢掉，`T = int`。（这和 C 里「函数改不了按值传入的实参」是同一个道理，只是上升到了类型推导层面。）

**规则 2：按引用传参（`T& x`），const 保留进 T。**

```cpp
static_assert(std::is_same_v<decltype(probeRef(i)),  int&>);        // T = int
static_assert(std::is_same_v<decltype(probeRef(ci)), const int&>);  // T = const int
```

为什么保留？引用直通原物，没有拷贝。如果把 `const int` 传给 `T&` 时 const 被丢掉，模板内部就能借引用改一个 const 对象——类型系统直接破防。所以 const 必须守住：`T` 推导成 `const int`，形参就是 `const int&`。

**规则 3：数组和函数按值传参时退化（decay）成指针；`T&` 能保住它们。**

```cpp
int arr[5] = {};
int takeInt(int);

static_assert(std::is_same_v<decltype(probeVal(arr)),     int*>);        // 数组退化
static_assert(std::is_same_v<decltype(probeVal(takeInt)), int(*)(int)>); // 函数退化
static_assert(std::is_same_v<decltype(probeRef(arr)),     int(&)[5]>);   // T& 保住长度！
```

数组退化你在 C 里天天见（数组传参变指针、`sizeof` 失效），模板按值推导原样继承了这条 C 规则，坑也原样继承：`probeVal(arr)` 里 `T = int*`，**数组长度 5 丢了**。而按引用传参时数组不退化，`T = int[5]`，长度完好——标准库能写出 `template <typename T, size_t N> size_t size(T (&)[N])` 这种「自动数出数组长度」的函数，靠的就是规则 3 的后半句。

> 这三条规则不止用在模板上。`auto` 的推导就是这一套（第 10 节），所以现在多花五分钟，之后 `auto` 白送。

### 3.2 推导冲突：同一个 T 只能是一种类型

```cpp
myMax(3, 5.0);   // 反例：编译错误
```

编译器视角：第一个实参说 `T = int`，第二个说 `T = double`。**推导只负责如实汇报，不负责和稀泥**——它不会「取个大的」自动折中成 double（那是隐式转换干的事，推导阶段不做转换）。两份汇报打架，推导失败。g++ 的报错很直白：

```
error: no matching function for call to 'myMax(int, double)'
note: candidate: 'template<class T> T myMax(T, T)'
note:   deduced conflicting types for parameter 'T' ('int' and 'double')
```

三种修法，按场景选：

```cpp
myMax(3.0, 5.0);          // 修法 1：实参统一类型
myMax<double>(3, 5.0);    // 修法 2：显式指定 T，跳过推导，int 隐式转 double
                          // 修法 3：模板改成两个类型参数（见 3.4）
```

### 3.3 显式指定：跳过推导，直接点菜

尖括号显式写类型，就是告诉编译器「别猜了，T 就是它」。两种情况必须显式写：

```cpp
template <typename T>
T parseAs(const std::string& s);   // T 只出现在返回类型里

auto v = parseAs<int>("42");       // 推导只看实参，实参里没有 T → 推不出，必须显式
```

一是像上面这样 `T` 无处可推；二是像 3.2 那样推导冲突需要仲裁。其余情况让编译器推，少写少错。

### 3.4 多个类型参数

模板头里可以放多个占位符：

```cpp
template <typename T, typename U>
auto add(T a, U b) {        // 返回类型交给 auto 从 return 推（C++14 起）
    return a + b;
}

add(3, 5.0);    // T = int，U = double，各推各的，不冲突
```

`int + double` 的结果类型是 `double`（C 的算术提升规则），写不出一个固定返回类型，所以交给 `auto` 从 `return` 语句推导。C++11 时代的等价写法是尾置返回类型 `auto add(T a, U b) -> decltype(a + b)`，STL 源码里常见，看懂即可。`std::pair<K, V>`、`std::map<K, V>` 就是双类型参数的类模板。

---

## 4. 类模板：生成类的配方

### 4.1 从 IntStack 到 Stack\<T\>

函数模板是「生成函数的配方」，类模板就是「生成类的配方」。回忆 M2 那个只能装 `int` 的栈，泛型化只需要一步：把写死的 `int` 换成空槽 `T`。

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

Stack<int>         si;   // 实例化出「装 int 的栈」这个类
Stack<std::string> ss;   // 实例化出「装 string 的栈」这个类
```

注意和函数模板的差别：**类模板实例化时通常要显式写 `<int>`**。函数模板能从实参推导，类模板在 C++17 之前没这个待遇（C++17 加了 CTAD——类模板实参推导，`std::pair p{1, 2.0}` 能自动推出 `pair<int, double>`，知道有这回事即可，入门阶段手写清楚更稳）。

### 4.2 `Stack<int>` 和 `Stack<double>` 是两个类型——推论三第一次现身

回到第 2.3 节的表：每种 `T` 各生成一份代码。所以 `Stack<int>` 和 `Stack<double>` 是**两个完全独立的类**，独立程度和你手写 `IntStack`、`DoubleStack` 两个类一模一样：

- 不能互相赋值，不能互相转换；
- 各自的成员函数是独立的两套机器码；
- 一个的私有成员对另一个也是私有的（它们连朋友都不是）。

`Stack` 本身**不是类型**，`Stack s;` 是编译错误——配方不是菜。这个「每个实例是独立类型」的事实还有个工程后果：**代码膨胀（code bloat）**。一个大模板被 30 个类型实例化，二进制里就有 30 份代码，编译时间和体积一起涨。入门阶段不用优化它，但要知道成本在哪——这是「编译期生成」的账单。

### 4.3 成员函数在类外定义：语法税

成员函数也可以放到类外定义，但每个定义都要交两笔「语法税」：重复模板头、类名写全：

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

template <typename T>               // 税 1：每个类外定义都要带模板头
void Stack<T>::push(const T& v) {   // 税 2：类名是 Stack<T>，不是 Stack
    data_[top_++] = v;
}

template <typename T>
T Stack<T>::pop() {
    return data_[--top_];
}
```

为什么要交税？因为**成员函数的定义本身也是一份配方**（它体内有 `T`），脱离了类的花括号，编译器不知道这里的 `T` 是谁家的，必须重新声明（`template <typename T>`）并指明归属（`Stack<T>::`）。这些类外定义**仍然必须和类待在同一个头文件里**——原因第 6 节推导给你。

### 4.4 默认模板参数

模板参数可以有默认值，规则和函数默认参数一样（从右往左给）：

```cpp
template <typename T, int N = 8>
class Buffer {
    T data_[N];
};

Buffer<int>     b1;   // N = 8
Buffer<int, 32> b2;   // N = 32
```

你天天在用而不自知：`std::vector<int>` 的全名是 `std::vector<int, std::allocator<int>>`，第二个参数（分配器）有默认值所以你从来不写。**STL 报错里那些吓人的超长类型名，就是编译器把所有默认参数展开后的全名**——记住这点，第 9 节读报错时能自动忽略一大半噪音。

### 4.5 类模板的友元 `operator<<`（mini 项目要用）

给 `Stack<T>` 重载 `operator<<` 打印内容，它要访问私有成员，得做友元。最省事且不易错的写法：**直接在类内定义友元函数**：

```cpp
#include <iostream>

template <typename T, int N = 64>
class Stack {
public:
    bool push(const T& v) {
        if (top_ == N) return false;
        data_[top_++] = v;
        return true;
    }
    // 类内定义的友元：随 Stack<T,N> 一起实例化，T、N 直接可用
    friend std::ostream& operator<<(std::ostream& os, const Stack& s) {
        for (int i = 0; i < s.top_; ++i) os << s.data_[i] << ' ';
        return os;
    }
private:
    T   data_[N];
    int top_ = 0;
};
```

类内的 `Stack` 自动等于 `Stack<T, N>`，不用写全。每实例化一个 `Stack<T,N>`，就顺带生成一个配套的非模板 `operator<<`，干净利落。（把友元声明在类内、定义放类外的写法也存在，但要处理「友元模板」的声明语法，容易错，入门用类内定义即可。）

---

## 5. 非类型模板参数：值也能进类型

### 5.1 配方的空槽不一定是类型

到目前为止空槽 `T` 填的都是类型。空槽还可以要一个**编译期常量值**（整数、枚举、指针等），最常用的是整数当数组大小：

```cpp
template <typename T, int N>   // T 填类型，N 填一个编译期 int 值
class Array {
public:
    T&       operator[](int i)       { return data_[i]; }
    const T& operator[](int i) const { return data_[i]; }
    int      size() const            { return N; }
private:
    T data_[N];               // N 编译期已知 → 栈上定长数组，合法
};

Array<double, 16> a;
Array<int, 3>     b;
```

`N` 必须编译期可知——第 2 节的五步流程发生在编译期，第 4 步「代入生成」时 `N` 就要有确定的值。所以：

```cpp
Array<int, 3> ok;          // 字面量，编译期常量，行
constexpr int M = 4;
Array<int, M> ok2;         // constexpr，编译期常量，行
int n = 5;
Array<int, n> bad;         // 反例：n 是运行期变量，编译器在第 4 步拿不到值
```

### 5.2 `std::array<int, 10>` 的 10 去哪了？——进了类型

这是理解非类型参数的钥匙。问你一个问题：`std::array<int, 10>`（就是上面 `Array` 的标准库版）的对象里，10 存在哪个成员变量里？

**答案：哪都没存。10 不在对象里，在类型名里。** `Array<int, 10>` 和 `Array<int, 11>` 是两个不同的类——就像 `Stack<int>` 和 `Stack<double>` 是两个类一样，模板参数（不管是类型还是值）不同，实例化出来的就是不同类型。可以编译期验证：

```cpp
#include <array>
#include <type_traits>

static_assert(!std::is_same_v<std::array<int,10>, std::array<int,11>>);  // 不同类型
static_assert(sizeof(std::array<int,10>) == 10 * sizeof(int));  // 对象里就是 10 个 int，没有额外字段
```

第二个断言说明对象内存里**只有 10 个 int，没有长度字段**（主流实现皆如此）。`size()` 返回的 10 是硬编码在 `Array<int,10>::size()` 这份生成代码里的常量，不是从内存里读的。

### 5.3 于是「零开销」水到渠成

对比三种「带大小的数组」：

| | C 裸数组 `int a[10]` | `std::array<int,10>` | C 的 `void*` + 手传 size |
|---|---|---|---|
| 大小在哪 | 在类型里（但传参就退化丢失） | **在类型里，且传参不丢**（传引用） | 在一个运行期变量里 |
| 内存开销 | 10 个 int | 10 个 int，分毫不差 | 数据 + size 变量 |
| 越界/尺寸错 | 编译器不管 | 类型不匹配直接编译错 | 运行期才崩 |

`std::array` 和裸数组同内存、同速度，却多了类型安全——因为「大小」这个信息从运行期数据搬进了编译期类型，检查全部提前到编译期做完，运行期一分钱不花。这就是 C++ 反复吹的「零开销抽象」在模板上的典型体现：**用编译期的工作量换运行期的零成本**。

---

## 6. 推论一讲透：为什么模板定义必须放头文件

从 C 过来**最痛的一坑**，也是练习 8 要你亲手炸一次的坑。别背结论，我们从 C 的编译模型出发把它推出来——推导链你在 C 里每一环都熟。

### 6.1 地基：编译单元各自为政，链接器只做符号拼接

回忆 C 的多文件编译（C++ 完全一样）：

1. 每个 `.cpp` 连同它 include 的头文件，独立编译成一个 `.obj`。**编译 a.cpp 时，编译器完全看不见 b.cpp 的内容**——不是不想看，是流程上根本不在场。
2. 编译单个 `.cpp` 时遇到「调用了别处定义的函数」，编译器只需要声明（签名）就能生成调用代码，同时在 `.obj` 里记一笔：「我引用了符号 `add(int,int)`，谁有实体链接时给我接上」。
3. 链接器把所有 `.obj` 摊开，**拿引用找实体，做符号拼接**。它蠢得很纯粹：不懂 C++、不懂模板、不会生成任何代码，只会连线。哪个引用找不到实体，就报「无法解析的外部符号」。

普通函数在这套模型里岁月静好：

```cpp
// util.h
int add(int a, int b);                       // 声明
// util.cpp
int add(int a, int b) { return a + b; }      // 定义 → util.obj 里有实体
// main.cpp
#include "util.h"
int r = add(1, 2);                           // main.obj 里记一笔引用
```

链接器拿 `main.obj` 的引用去 `util.obj` 找实体，找到，接上，完事。**这套「声明放 .h、定义放 .cpp」的习惯能成立，前提是：定义那边不需要知道使用方的任何信息，编译 util.cpp 时就能独立产出实体。**

### 6.2 模板恰好打破了这个前提

模板的实体（实例）要等到「有人拿具体类型来用」才生成（第 2 节第 4 步），而且生成时必须看到配方全文。现在按 C 的习惯把模板拆开，推演一遍会发生什么：

```cpp
// adder.h —— 只有声明
#pragma once
template <typename T>
T addAll(const T* arr, int n);

// adder.cpp —— 定义在这
#include "adder.h"
template <typename T>
T addAll(const T* arr, int n) {
    T sum{};
    for (int i = 0; i < n; ++i) sum += arr[i];
    return sum;
}

// main.cpp —— 使用在这
#include "adder.h"
int main() {
    int a[3] = {1, 2, 3};
    return addAll(a, 3);      // 需要 addAll<int>
}
```

两个编译单元各自会发生什么，逐个过：

**编译 main.cpp**：读到 `addAll(a, 3)`，推导 `T = int`，走到第 4 步「代入生成」——坏了，手里只有一行声明，**配方全文（函数体）在 adder.cpp 里，本编译单元看不见**。生成不了。编译器退而求其次，当普通函数调用处理：记一笔对符号 `addAll<int>` 的引用，寄希望于别的 `.obj` 里有实体。`main.obj` 编译**通过**（这最迷惑人——编译不报错！）。

**编译 adder.cpp**：配方全文在手，万事俱备。但是——**本编译单元里没有任何人用 `addAll`**。惰性实例化：没有实例化请求，一份实体都不生成。`adder.obj` 编译**也通过**，但里面关于 `addAll` 是空的。

**链接**：链接器拿着 `main.obj` 里的引用「`addAll<int>` 的实体在哪」，翻遍所有 `.obj`——没有。它自己又不会生成（它连模板是什么都不知道）。于是：

MSVC（练习 8 你会看到的）：

```
ex8_main.obj : error LNK2019: 无法解析的外部符号
    "int __cdecl addAll<int>(int const *,int)" (??$addAll@H@@YAHPEBHH@Z)，
    函数 main 中引用了该符号
```

g++ 同款：

```
undefined reference to `int addAll<int>(int const*, int)'
```

看清楚这场事故的结构：**想生成的（main.cpp）看不到配方，看得到配方的（adder.cpp）没人让它生成，链接器只会连线不会做菜。三方各自「正确」，合起来无解。** 而且错误在链接期才爆，报的是符号名不是行号，第一次撞上时极难反应过来根因是「模板定义放错了地方」。

### 6.3 解法：让配方跟着使用点走——放头文件

既然「每个使用模板的编译单元都必须看到配方全文」，那配方就放在人人都会 include 的地方——头文件：

```cpp
// adder.h —— 声明和定义都在头文件
#pragma once
template <typename T>
T addAll(const T* arr, int n) {
    T sum{};
    for (int i = 0; i < n; ++i) sum += arr[i];
    return sum;
}
```

现在每个 include 它的 `.cpp` 都握有配方全文，谁用到什么类型就地实例化，链接器收到的全是有实体的符号。你可能会问：main.cpp 和 other.cpp 都实例化了 `addAll<int>`，符号不就重复了？——模板实例是「弱符号」，链接器认得这种标记，多份相同实例只保留一份，不算重定义。这套机制就是为模板设计的。

> 铁律：**函数模板、类模板及其全部成员函数的定义，都放头文件。** 这也解释了一个你可能纳闷过的现象：STL 为什么全是头文件？`<vector>`、`<algorithm>` 里全是模板，根本没有对应的 `.cpp` 可言。

**逃生门（了解即可）**：如果实在想把定义藏进 `.cpp`（比如想隐藏实现、或压编译时间），可以在 adder.cpp 末尾写**显式实例化**：`template int addAll<int>(const int*, int);`——手动命令编译器「就在这个编译单元，把 int 版实体生成出来」。代价是所有会用到的类型都得提前列全，泛型的开放性没了。知道有这扇门就行，入门阶段用不上。

---

## 7. 推论二讲透：惰性实例化

### 7.1 用到才生成，没用到的连检查都不做

「第 3、4 步：用到才生成」展开说就是：**模板对没被用到的部分，既不生成代码，也不做类型检查**（只做最基本的语法检查，比如括号配不配对）。函数模板层面很好理解——你没调过 `myMax<double>`，二进制里就没有它。真正有意思的是类模板。

### 7.2 类模板的成员函数：一个一个单独惰性

`Stack<int> s;` 实例化的是什么？只是类本身（成员变量布局、对象大小这些「壳」）。**每个成员函数是一份独立的小配方，谁被调用谁才实例化。** 看这个能编译验证的例子：

```cpp
struct NoCompare {
    int v = 0;
    // 故意不提供 operator==
};

template <typename T>
class Box {
public:
    void set(const T& x) { val_ = x; }
    bool equals(const T& x) const { return val_ == x; }  // 体内用了 ==
private:
    T val_{};
};

int main() {
    Box<NoCompare> b;       // 实例化 Box<NoCompare> 的壳：没问题
    b.set(NoCompare{1});    // 实例化 set：只用到拷贝赋值，NoCompare 有 → 编译通过
    // b.equals(NoCompare{1});  // 反例：取消注释才实例化 equals → 才发现没有 == → 才报错
}
```

`Box` 的配方里明晃晃写着 `val_ == x`，而 `NoCompare` 根本没有 `==`——但只要你不调 `equals`，这段代码**永远不会被检查**，整个程序编译运行毫无异样。取消注释的瞬间，g++ 报：

```
neg3_lazy.cpp: In instantiation of 'bool Box<T>::equals(const T&) const [with T = NoCompare]':
error: no match for 'operator==' (operand types are 'const NoCompare' and 'const NoCompare')
```

注意第一行的 `In instantiation of`：报错发生在**实例化 equals 的那一刻**，不是定义 `Box` 的时候——推论二和推论四在同一条报错里碰头了。

### 7.3 这解释了 STL 的一个「灵异现象」

想过没有：`std::vector<T>` 支持 `v1 == v2` 整体比较（要求元素有 `==`）、支持 `std::sort`（要求元素有 `<`），那把一个既没 `==` 也没 `<` 的结构体塞进 vector，为什么好好的？

```cpp
struct NoCompare { int v = 0; };            // 没有 == 也没有 <

std::vector<NoCompare> v;
v.push_back(NoCompare{2});                  // 完全没问题
// bool same = (v == v);                    // 反例：这行才要求元素有 ==，才炸
```

答案就是惰性实例化：`vector` 的上百个成员函数里，你只用了 `push_back` 等几个，只有它们被实例化；`operator==` 那份配方安静地躺着，`NoCompare` 缺 `==` 的事没人过问。**vector 对元素类型的要求不是一张总清单，而是「用哪个功能，交哪个功能的税」。** 这是 STL 泛用性的重要来源——同一个 vector，能装要求五花八门的类型，各取所需。

顺带兑现第 2.3 表格的一条：这也是「模板代码里的错误可以长期潜伏」的原因——某个类型组合的 bug，只要没人实例化那条路径，测试全绿。模板库的测试要刻意把每个成员、每类典型类型都实例化一遍，就是这个道理。

---

## 8. 特化：给配方开小灶

### 8.1 通用配方也有失手的时候

配方对「大多数 T」正确，不代表对所有 T 正确。经典翻车现场：

```cpp
template <typename T>
bool equal(T a, T b) { return a == b; }

equal(1, 1);        // 对
equal("hi", "hi");  // T = const char*，比较的是两个指针的地址！内容相等与否全看运气
```

对 `const char*`，`==` 比地址不比内容——通用配方的逻辑在这个类型上语义就是错的。C 里你只能换函数名（`strcmp` 另立门户），C++ 允许你**给特定类型单独开小灶**，用的人无感知，这就是**特化（specialization）**。

### 8.2 全特化：指名道姓一个类型

```cpp
#include <cstring>

template <typename T>
bool equal(T a, T b) { return a == b; }        // 主模板（primary template）

template <>                                     // 全特化：模板头空了——没有空槽要填
bool equal<const char*>(const char* a, const char* b) {
    return std::strcmp(a, b) == 0;              // 比内容
}

equal(1, 1);         // 走主模板
equal("hi", "hi");   // 走特化版（字符串字面量推导为 const char*）
```

语法记两点：`template <>` 空模板头（类型已完全指定，无槽可填），函数名/类名后跟 `<具体类型>`。类模板同理：

```cpp
template <typename T>
struct TypeName {
    static const char* get() { return "unknown"; }
};

template <>
struct TypeName<int> {                          // 为 int 单开一份类定义
    static const char* get() { return "int"; }
};
```

注意全特化的类是**从头另写的一份**，和主模板可以毫无相似之处（成员都可以完全不同）——它不是「继承后微调」，是「整个换掉」。

### 8.3 偏特化：给「一类类型」开小灶（类模板专属）

全特化指名道姓一个类型，**偏特化（partial specialization）**圈住一类类型——最常见的是「所有指针」：

```cpp
template <typename T>
struct Inspect {                        // 主模板：兜底
    static const char* kind() { return "value"; }
};

template <typename T>
struct Inspect<T*> {                    // 偏特化：匹配一切指针类型
    static const char* kind() { return "pointer"; }
};

template <typename T, std::size_t N>
struct Inspect<T[N]> {                  // 偏特化还能带自己的参数：匹配一切定长数组
    static const char* kind() { return "array"; }
};

Inspect<int>::kind();      // "value"
Inspect<double*>::kind();  // "pointer"
Inspect<int[4]>::kind();   // "array"
```

认语法：偏特化的模板头**不为空**（还剩空槽 `T`），但类名后面挂了一个**模式**（`Inspect<T*>`）——「T 随便，但整体必须长成 T* 的样子」。模式里可以引入自己的参数（`T[N]` 里的 `N`），练习 7 就考这个。这套「按类型的形状分派逻辑」是标准库类型萃取（type traits，`std::is_pointer` 之流）的实现根基，M7 之后你会常打照面，现在能看懂就够。

### 8.4 编译器怎么选：具体的赢过泛化的

同一个 `Inspect<int*>`，主模板、`T*` 偏特化都能匹配，如果再写一个 `template <> struct Inspect<int*>` 全特化，三个都能匹配。编译器按一条原则选：**谁描述得更具体，谁上。**

```
全特化（指名道姓）  >  偏特化（圈住一类）  >  主模板（来者不拒）
```

```cpp
template <typename T> struct Pick       { /* A */ };   // 主模板
template <typename T> struct Pick<T*>   { /* B */ };   // 偏特化
template <>           struct Pick<int*> { /* C */ };   // 全特化

Pick<double>  → A（只有主模板能匹配）
Pick<char*>   → B（偏特化能匹配，比主模板具体）
Pick<int*>    → C（三个都匹配，全特化最具体）
```

多个偏特化都能匹配时怎么比？直觉版规则：**A 模式能匹配的集合如果真包含于 B 模式能匹配的集合，B 更特殊，B 赢**（`const T*` 比 `T*` 特殊，因为凡是匹配 `const T*` 的都匹配 `T*`，反之不然）。分不出高下就报二义性错误。够用即止——日常写代码的层次结构都很浅，一眼看得出谁更具体。

### 8.5 函数模板没有偏特化——用重载顶替

语言规定：**偏特化只给类模板，函数模板没有**。因为函数天生就有一套更顺手的机制——重载：

```cpp
template <typename T> const char* describe(T)  { return "value"; }
template <typename T> const char* describe(T*) { return "pointer"; }  // 这是重载，不是偏特化

int n = 0;
describe(n);    // "value"
describe(&n);   // "pointer"——重载决议选中更匹配的那个
```

两个独立的函数模板构成重载，重载决议同样偏爱更具体的匹配，效果和偏特化几乎一样，语法还更自然。记住口诀：**类模板用偏特化，函数模板用重载。** 面试爱问「函数模板能偏特化吗」，答案：不能，用重载代替。

---

## 9. 推论四讲透：报错为什么长，怎么读

### 9.1 报错冗长不是编译器坏，是模型使然

把推论三、四合起来，STL 报错的两大特征都有了出处：

- **错误爆在模板内部**：类型检查推迟到实例化点（第 2 节第 5 步），而实例化点往往在 STL 源码深处——你调 `std::sort`，sort 里调 `__introsort_loop`，再调 `__unguarded_linear_insert`，最里层那句 `*__i < __val` 才发现你的类型没有 `<`。错误位置：标准库头文件第 1757 行，不是你的代码。
- **编译器打印整条实例化链**：只报「stl_algo.h:1757 出错」你根本没法定位，所以编译器把「谁实例化了谁」的调用链从头到尾列出来（每层一句 `required from ...`），再加上默认模板参数全部展开的超长类型名（4.4 节），一个错几十行就这么来的。

所以那几十行不是几十个错，是**一个错 + 一条通往你代码的路标链**。会认路标就不可怕。

### 9.2 实战解剖一条真报错

```cpp
#include <algorithm>
#include <vector>
struct P { int x; };                     // 反例：没有 operator<
int main() {
    std::vector<P> v(3);
    std::sort(v.begin(), v.end());       // sort 要用 < 比较元素
}
```

g++ 吐出几十行，抽出骨架（省略号是我删掉的噪音）：

```
stl_algo.h:1777:   required from 'void std::__insertion_sort(...)'
stl_algo.h:4771:   required from 'void std::sort(_RAIter, _RAIter) [with _RAIter = ...<P*, vector<P>>]'
neg2_sort.cpp:6:14:   required from here          ← ★ 你的代码在这
predefined_ops.h:45: error: no match for 'operator<' (operand types are 'P' and 'P')   ← ★ 病因在这
```

MSVC 的等价核心行长这样：

```
error C2676: 二进制“<”:“const P”不定义该运算符或到预定义运算符可接收的类型的转换
```

两个 ★ 就是全部信息：**你的哪一行**（`required from here` 指向的 `neg2_sort.cpp:6`，MSVC 里是「查看对正在编译的函数模板实例化的引用」那行）触发的，**缺什么**（`P` 没有 `operator<`）。修复：给 `P` 加 `operator<`，或给 `sort` 传第三个参数（自定义比较器）。

### 9.3 通用套路，四条

1. **只修第一个 `error:`**。后面的 error 多半是第一个的连锁塌方，修好第一个重编，经常一片绿。
2. **找 `required from here` / 「引用」链里属于你自己文件的那行**——从报错底部往上扫，文件名不是标准库路径的就是它。那是导火索，光标先跳过去。
3. **`std::` 内部的帧全部跳过，长类型名心里折叠**：`std::vector<P, std::allocator<P>>` 读作 `vector<P>`，`std::__cxx11::basic_string<char, ...>` 读作 `string`，`__gnu_cxx::__normal_iterator<P*, ...>` 读作「vector\<P\> 的迭代器」。双下划线开头的一律是实现细节，不是你的错。
4. **抓 error 行的关键词短语**：`no match for 'operator<'`（缺运算符）、`no member named`（缺成员）、`no matching function`（没有匹配的重载/推导失败）、`static assertion failed`（库主动检查你违反了什么前提，通常附人话说明）。

90% 的 STL 报错，翻译成人话都是同一句：**「你给的类型不满足这个模板用到的某个操作」**——这正是推论二的镜像：用到哪个功能，才要求哪个能力；要求不满足，实例化那刻报错。练习 8 加上平时多炸几次，一周就有肌肉记忆。

---

## 10. 模板家族的延伸：auto、decltype、if constexpr、变参

这四样东西看着散，其实全长在「编译期生成与推导」这同一棵树上。

### 10.1 `auto` 就是模板推导——3.1 节的规则白送

```cpp
auto x = 3 + 5;   // 推导出 int
```

`auto x = expr;` 的推导规则和 `template <typename T> void f(T x)` 遇到 `f(expr)` 推 `T` 是**同一套**（标准原文就是这么定义的）。所以 3.1 的三条规则直接搬过来：

```cpp
const int ci = 0;
auto  a = ci;    // 按值模式：剥 const → int（对应规则 1）
auto& b = ci;    // 引用模式：保留 const → const int&（对应规则 2）
int arr[5];
auto  c = arr;   // 退化 → int*（对应规则 3）
auto& d = arr;   // 保住 → int(&)[5]
```

以后见到 `auto` 拿不准推成什么，就在心里把它替换成模板参数 `T`，按 3.1 走一遍。

### 10.2 `decltype`：只问类型，不做求值

`decltype(表达式)` 给你表达式的类型，表达式本身不执行。模板里的典型用途是「返回类型依赖参数、写不出来」的场合（3.4 节的 `-> decltype(a + b)`）。和 `auto` 分工：`auto` 是「按推导规则算给我」（会剥会退化），`decltype` 是「原样报给我」（const、引用一根毛都不动）。入门记住「模板里返回类型写不出来时，C++14 用 `auto`，读老代码认得 `-> decltype(...)`」即可。

### 10.3 `if constexpr`：惰性实例化思想长进了函数体

C++17 的 `if constexpr` 在**编译期**判断条件并**裁掉**不满足的分支。它和普通 `if` 的区别不是快慢，是本质性的：**普通 `if` 的两个分支都必须编译通过（都会被实例化），`if constexpr` 不满足的分支根本不实例化**——推论二（用不到的不生成不检查）在函数体内部的翻版：

```cpp
#include <type_traits>

template <typename T>
auto valueOf(T x) {
    if constexpr (std::is_pointer_v<T>) {
        return *x;      // 只有 T 是指针时这个分支才实例化
    } else {
        return x;
    }
}

int n = 42;
valueOf(&n);   // 生成的代码只有 return *x;
valueOf(n);    // 生成的代码只有 return x; —— *x 那行不存在，不会因“int 不能解引用”报错
```

把 `constexpr` 拿掉换成普通 `if` 就编译不过：`valueOf(n)` 实例化时两个分支都要检查，`*x` 对 `T = int` 非法。**一份配方按类型生成不同形状的代码**，`if constexpr` 是模板元编程平民化的第一步，M7、M8 会再遇到。

### 10.4 变参模板与折叠表达式（够用即止）

空槽的个数也能不定——**变参模板（variadic template）**，`...` 表示「一包」：

```cpp
#include <iostream>

template <typename... Args>            // Args：模板参数包（一包类型）
void print(const Args&... args) {      // args：函数参数包（一包参数）
    (std::cout << ... << args) << "\n";   // C++17 折叠表达式：把 << 依次套到整包上
}

print(1, " hello ", 3.14);   // 一次调用，任意个数、任意类型
```

编译器视角依旧是那五步：`print(1, " hello ", 3.14)` 推导出 `Args = {int, const char*, double}`，代入生成一份三参数的具体函数。折叠表达式 `(std::cout << ... << args)` 展开成 `((std::cout << 1) << " hello ") << 3.14`。这就是 `printf` 的类型安全升级版：`printf` 的 `...` 是 C 变参，类型信息全丢、格式串对不上就是未定义行为；变参模板每个参数类型都在编译期一清二楚。

入门到此即可：认得 `typename... Args` 和 `(op ... op args)`，知道 `std::make_unique`、`emplace_back` 这类「把任意参数原样转交给构造函数」的接口就是变参模板 + 完美转发（见第 15 节）实现的。

---

## 11. vs C 全景对比表

把全模块的对比收拢成一张表，复习用：

| 维度 | C 宏 | C `void*` | C++ 模板 |
|---|---|---|---|
| 机制 | 预处理期文本替换 | 运行期类型擦除 | **编译期代码生成** |
| 类型检查 | 无 | 无（强转听天由命） | 编译期严格检查 |
| 类型信息 | 不感知 | 主动丢弃 | 完整保留 |
| 运行期开销 | 无 | 函数指针调用、不可内联 | **零**（生成的是普通代码，可内联） |
| 副作用 | 参数可能重复求值 | — | 真函数，求值一次 |
| 作用域 | 无视 | 遵守 | 遵守 |
| 调试 | 展开后无踪迹 | 可调试但类型盲盒 | 普通函数/类，可断点单步 |
| 值参数化 | 能（但同样无检查） | 手传 size，错了运行期崩 | 非类型参数，进类型，编译期验证 |
| 代价 | 可读性、安全性 | 性能、安全性 | **编译时间、二进制体积、报错长度** |

最后一行别跳过：模板不是免费午餐，它把成本从运行期挪到了编译期——这笔交易几乎总是划算，但要知道自己付了什么。

---

## 12. 常见坑

1. **模板定义放进 `.cpp`** → 编译全过、链接报 LNK2019 / `undefined reference`。头号大坑，根因见第 6 节：用的人看不到配方，看到配方的没人让它生成。定义放头文件。
2. **推导冲突**：`myMax(3, 5.0)` 报 `no matching function`——推导不做类型转换，两个实参给同一个 `T` 报了不同的类型。统一实参、显式 `myMax<double>(...)`、或改双参数模板。
3. **类模板忘写类型参数**：`Stack s;` 错——`Stack` 是配方不是类型，`Stack<int> s;` 才是（C++17 CTAD 能省的场合先不依赖）。
4. **`Stack<int>` 和 `Stack<double>` 当同一类型**：它们是两份独立生成的类，不能互相赋值。同理 `Array<int,3>` 和 `Array<int,4>` 也是两个类型——值参数也参与「类型身份」。
5. **非类型参数传运行期变量**：`int n = 5; Array<int, n> a;` 错，实例化发生在编译期，`N` 必须是编译期常量（字面量、`constexpr`）。
6. **想给函数模板偏特化**：语言不支持，用重载（`describe(T)` + `describe(T*)`）。偏特化是类模板专属。
7. **按值传数组给模板，长度悄悄丢了**：`T x` 推导时数组退化成指针（3.1 规则 3），`sizeof` 全错。要保长度用 `T (&)[N]` 或传 `std::array`。
8. **以为「编译过 = 模板没问题」**：惰性实例化意味着没用到的成员函数从未被检查，错误可以潜伏到第一次调用才炸（7.2 节）。模板代码要用典型类型把各成员都实例化一遍才算测过。
9. **被几十行报错吓住**：那是一个错 + 一条实例化路标链。按 9.3 四步走：第一个 error、找自己的文件行、折叠 `std::` 噪音、抓关键词。
10. **代码膨胀无感知**：几十个类型实例化一个大模板，编译肉眼变慢、二进制变大。入门不用优化，心里有账。

---

## 13. 高频面试点（附答案要点）

- **模板和宏的区别？** 宏是预处理期文本替换：无类型检查、参数重复求值、无作用域、无法调试；模板是编译期类型安全的代码生成，产物是真函数/真类，零运行期开销。一句话：文本替换 vs 代码生成。
- **模板和 `void*` 泛型的区别？** `void*` 靠丢弃类型换通用：手传 size、满屏强转、函数指针回调不可内联；模板保留完整类型信息，每个类型一份专属代码，可内联、零开销、编译期查错。
- **模板什么时候实例化？什么是惰性实例化？** 用到具体类型才实例化；类模板的成员函数逐个惰性——没被调用的成员不生成也不做类型检查。所以 `vector<T>` 能装没有 `==` 的 T，只要你不比较。
- **为什么模板定义要放头文件？** 实例化时编译器必须看到完整定义才能生成代码；定义放 `.cpp` 时，使用方的编译单元只有声明生成不了，定义方的编译单元没有使用请求不会生成，链接器只拼符号不生成代码，结果 LNK2019/undefined reference。（加分项：显式实例化可以作为例外手段。）
- **全特化和偏特化的区别？函数模板能偏特化吗？** 全特化指定全部模板参数（`template <>`），为一个具体类型另写实现；偏特化保留部分参数、限定模式（如 `T*`），匹配一类类型。选择顺序：全特化 > 偏特化 > 主模板，越具体越优先。函数模板不能偏特化，用重载代替。
- **非类型模板参数是什么？`std::array` 为什么零开销？** 编译期常量值做模板参数。`array<int,10>` 的 10 进了类型而非对象内存——对象里就是 10 个 int，没有长度字段，`size()` 返回编译期常量；大小检查全部在编译期完成。
- **`typename` 和 `class` 在模板参数里的区别？** 声明类型参数时完全等价。（`typename` 另有指明依赖类型的用途，进阶话题。）
- **`auto` 推导和模板推导什么关系？** 同一套规则：按值剥引用剥顶层 const，`auto&` 保留 const，数组/函数按值退化成指针。
- **`if constexpr` 和普通 `if` 的本质区别？** 普通 `if` 两个分支都要实例化、都必须编译通过；`if constexpr` 在编译期裁剪，不满足的分支不实例化，允许分支里出现对当前 `T` 非法的代码。
- **模板的代价？** 编译变慢、二进制膨胀（每个类型一份实例）、错误推迟到实例化点导致报错冗长、未实例化路径的错误可以潜伏。

---

## 14. 编译提醒

单文件练习（x64 Native Tools 命令行）：

```
cl /EHsc /std:c++17 /W4 文件名.cpp
```

多文件（练习 8）：

```
cl /EHsc /std:c++17 /W4 ex8_main.cpp Adder.cpp
```

注意：头文件不出现在命令行里（它随 `#include` 参与每个 `.cpp` 的编译）。练习 8 的「报错版」编译时，两个 `.cpp` 都会**编译成功**，错误在最后的链接阶段以 `LNK2019 无法解析的外部符号` 出现——请把完整报错抄进注释，那是本模块最值钱的一条错误信息。

---

## 15. 承前启后

**回望 M5——伏笔兑现。** M5 结尾说过「模板中的 `T&&` 不是右值引用，是转发引用」，现在你有了接住它的全部零件：`T&&` 里的 `T` 要走实参推导（第 3 节），推导时左值让 `T` 变成 `T&`、经引用折叠 `T&& → T&`，右值则保持 `T&&`——所以同一个形参左右通吃。而 `std::forward<T>(arg)` 负责在转交参数时还原它原本的左右值属性：

```cpp
template <typename T>
void wrapper(T&& arg) {                 // 转发引用：左值右值都能接
    process(std::forward<T>(arg));      // 左值仍是左值，右值仍是右值
}
```

M5 你知道了移动语义「是什么」，M6 你明白了模板推导「怎么运转」，两者相乘就是完美转发——`std::make_unique`、`emplace_back` 能把任意参数原封不动交给构造函数，靠的就是「变参模板 + 转发引用 + `std::forward`」三件套。现在这行代码对你已经没有任何黑盒。

**展望 M7——这里配的钥匙。** M7 正式开用 STL：`vector`、`map`、`string`、迭代器、算法。它们**每一个都是模板**：`vector<int>` 是类模板实例（第 4 节）、`array<int, N>` 是非类型参数（第 5 节）、`sort` 的比较要求和报错长相（第 7、9 节）、`<vector>` 为什么是纯头文件（第 6 节）——M7 的每个「为什么」，答案都已经在本模块。你做的 mini 项目 `Stack<T, N>` 就是 STL 容器的素坯：M7 你会看到 `std::vector` 不过是「写好了、优化过、装上迭代器」的同类模板容器。理解了配方，就不怕任何照配方做出来的菜。

下一步：打开 `exercises.md`。重点照顾练习 8——亲手把链接坑炸响一次再修好，胜过重读第 6 节十遍。
