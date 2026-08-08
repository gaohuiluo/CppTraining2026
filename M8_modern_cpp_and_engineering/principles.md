# M8 现代特性与工程化：把「约定」变成「保证」

> 目标：这是 M1~M8 的收官模块。前面七个模块你把类、RAII、拷贝移动、模板、STL 打通了，这一模块回答一个贯穿性的问题——**怎么把 C 里靠约定、靠注释、靠人肉纪律保证的东西，变成类型系统和工具链的保证。**
>
> 你写了多年 C，一定熟悉这些「纪律」：返回 -1 表示失败，记得检查；union 旁边配个 tag，记得同步更新；`char*` 参数不拥有内存，记得别存着；malloc 了记得 free，出错路径也要 free；共享变量加锁，记得别忘了哪个入口。这些纪律没有一条是编译器帮你执行的——全靠你和你的同事今天状态好。现代 C++ 的整个设计方向，就是把这些纪律逐条搬进类型系统和工具链，让「忘了」变成「编译不过」或「工具当场报警」。

---

## 0. 一句话总览

**本模块只有一条主线：把「靠人保证」升级为「靠编译器和工具保证」。** 四个部分是这条主线在四个领域的展开：

| 部分 | C 里靠什么保证 | C++ 把它变成什么 |
|---|---|---|
| 一、类型特性 | 约定：-1 表示没值、union 旁边有 tag、`char*` 只读别存、宏常量、裸 enum 就是 int | **把约定编码进类型**：`optional`（可能没值）、`variant`（多选一）、`string_view`（只读视图）、`constexpr`（编译期常量）、`enum class`（受限整数） |
| 二、错误处理 | 纪律：记得检查返回值、层层向上转发 | **不可忽略的控制流**：异常自动向上传播，配 RAII 保证出错不漏资源 |
| 三、并发 | 小心：共享数据前后记得 lock/unlock | **结构性保证**：RAII 锁忘不了解锁，`atomic` 由硬件保证不可分割 |
| 四、工程化 | 口头：「我这儿能编译」「我测过了」 | **可复现的流程**：CMake 描述构建、CTest 自动跑测试、Sanitizer 自动抓内存错误 |

每一节都按同一个节奏讲：**它解决什么问题（C 里的痛）→ 它内部是什么结构（机制）→ 怎么用（最小示例）**。机制这一步最重要——知道 `optional` 内部就是「一块 T 的空间加一个 bool」，它的一切行为（值语义、不堆分配、和指针的区别）都能自己推出来，不用背。

---

# 第一部分：把约定编码进类型

这一部分五个特性，表面上互不相干，其实是同一件事的五个方向：**C 里有一批「类型系统表达不了、只能靠约定补足」的概念，C++ 给每个概念造了一个专用类型。**

- 「这个值可能没有」→ `std::optional<T>`
- 「这个值是几种类型之一」→ `std::variant<Ts...>`
- 「这段字符只读、我不拥有」→ `std::string_view`
- 「这个值编译期就该定死」→ `constexpr`
- 「这个整数只能取这几个值」→ `enum class`

概念进了类型，编译器就看得见它，就能替你检查。

## 1. `std::optional<T>`：把「可能没有值」写进类型

### 1.1 问题：C 怎么表达「查不到 / 算不出」

C 的函数签名里没有「可能没结果」这个概念，只能用手段模拟，三招各有硬伤：

```c
// 招式一：哨兵值——挑一个"不可能的"返回值代表失败
int find_index(const int* arr, int n, int target);   // 找不到返回 -1

// 招式二：出参 + bool
bool parse_int(const char* s, int* out);             // 成功才写 *out

// 招式三：全局 errno
```

哨兵值的硬伤是**哨兵占用合法值域**：`find_index` 用 -1 还行，但如果函数返回的是温度、偏移量差值、任意 int 呢？你找不到一个「绝对不可能出现」的值当哨兵。而且哨兵长得和正常值一模一样——`int idx = find_index(...); arr[idx]` 忘了检查照样编译、照样跑，直到某天越界。

出参的硬伤是**调用方可以忘**：忘了检查返回的 bool，`*out` 里是垃圾；`out` 传 NULL 还可能直接崩。这两招的共同病根是：**「有没有值」这个信息不在类型里，编译器看不见，只能靠文档和自觉。**

### 1.2 机制：一块 T 的空间 + 一个 bool

`std::optional<T>` 的内存布局非常朴素——**它内部就是「一块能放 T 的存储空间」加「一个表示有没有值的 bool」**，两者并排放在 optional 对象自己体内：

```
optional<double> 的布局（示意）：
+------------------+------+-----+
| double 的存储空间 | bool | 填充 |
+------------------+------+-----+
  8 字节              1     7      → sizeof = 16
```

从这个布局能直接推出它的三条关键性质：

1. **值语义，不堆分配。** T 就存在 optional 体内，构造 `optional<int>` 不碰堆，拷贝它就是拷贝这块空间。它是个普通值，跟 `int` 一样传来传去。
2. **它和「用指针表示可空」本质不同。** `T*` 为空是「不指向任何地方」，值在别处、生命周期和指针无关；`optional<T>` 为空是「我体内这块空间还没构造 T」，值就在体内、生死随 optional。所以 optional 没有悬垂问题、没有「谁来 free」的问题——这是它比「返回指针、NULL 表示没有」干净的根本原因。
3. **空不占用 T 的值域。** 那个 bool 是额外的一位信息，T 的每个值都是合法的「有值」状态——哨兵值的硬伤被布局直接消解。

### 1.3 用法

```cpp
#include <iostream>
#include <optional>
#include <vector>

// 类型签名自己在说话："这个函数可能没有结果"
std::optional<size_t> findFirst(const std::vector<int>& v, int target) {
    for (size_t i = 0; i < v.size(); ++i)
        if (v[i] == target) return i;   // 有值：隐式包成 optional
    return std::nullopt;                // 没值
}

int main() {
    std::vector<int> v{3, 7, 42};

    if (auto pos = findFirst(v, 42)) {          // 转 bool：有值为 true
        std::cout << "找到，下标 " << *pos << "\n";   // 解引用取值
    }
    size_t p = findFirst(v, 99).value_or(0);    // 没值就用兜底值
    std::cout << "带兜底: " << p << "\n";

    // 布局验证：多出来的就是那个 bool（加对齐填充）
    std::cout << sizeof(double) << " -> " << sizeof(std::optional<double>) << "\n"; // 8 -> 16
}
```

要点：
- `if (opt)` 判断、`*opt` 取值、`value_or(默认)` 兜底、`std::nullopt` 表示空。
- ⚠️ 对空 optional 用 `*opt` 是**未定义行为**（那块空间里根本没构造过 T，读它和读野指针指向的内存同罪）；`opt.value()` 会检查、空时抛 `std::bad_optional_access`。先判断再取。
- 适用场景：「正常情况下就可能没结果」——查表未命中、解析失败、可选配置。**不是**用来报告严重错误的（那是异常的活，见第二部分）。

| | C 哨兵值 / 出参 | `std::optional<T>` |
|---|---|---|
| 「没值」的表达 | 占用值域 / 靠 bool 出参 | 类型自带，不占 T 的值域 |
| 忘记检查 | 静默编译，运行期出 bug | 签名醒目，`value()` 空时抛异常 |
| 内存 | — | 值语义、不堆分配 |
| 适用性 | 找不到哨兵就没辙 | 任何 T 都能包 |

## 2. `std::variant`：编译器替你维护 tag 的 union

### 2.1 问题：C 的 union + 手动 tag，两处随时会塌

C 里表达「这块内存有时是 int、有时是 double、有时是字符串」只有 union，而 union 不记得自己现在装的是谁，你得手动配 tag：

```c
enum Tag { TAG_INT, TAG_DBL, TAG_STR };
struct Value {
    enum Tag tag;              // 手动维护的"当前是谁"
    union { int i; double d; char* s; } u;
};

struct Value v;
v.tag = TAG_INT; v.u.i = 42;

double bad = v.u.d;            // 险一：按错误成员读，未定义行为，编译器不拦
v.u.d = 3.14;                  // 险二：改了内容忘了更新 tag —— 从此 tag 是谎言
```

两大险：**读错成员是 UB**（不是「读到乱码」这么客气，是标准直接不定义行为）；**tag 和内容靠手动同步**，任何一处赋值忘了改 tag，之后所有依赖 tag 的 switch 都在按谎言办事。这套模式里正确性完全押在「每个写它的人都记得规矩」上。

### 2.2 机制：最大成员的空间 + 一个类型下标

`std::variant<int, double, std::string>` 的布局：**一块「够放最大那个成员」的存储空间，加一个记录「当前装的是第几个类型」的下标（tag）**：

```
variant<int, double, string> 的布局（示意）：
+----------------------------------+--------+
| 存储空间（按最大成员 string 算）  | index  |
+----------------------------------+--------+
```

也就是说，**`variant` 就是那个「union + tag」结构体，只不过 tag 由编译器替你维护**：每次赋值，它构造新类型的同时自动更新下标；每次取值，它先核对下标，不对就抛 `std::bad_variant_access`。C 那两大险被机制直接堵死——tag 不可能忘更新（不归你管了），读错成员不可能静默（有人查岗）。

和 union 还有一个关键差别：union 装不了 `std::string` 这种有构造/析构的类型（谁负责析构说不清），variant 可以——它知道当前装的是谁，切换类型时先析构旧的再构造新的，析构时按下标析构正确的成员。

### 2.3 用法：取值三件套 + `visit`

```cpp
#include <iostream>
#include <string>
#include <variant>

using Value = std::variant<int, double, std::string>;

// visitor：一个能处理所有备选类型的可调用对象
struct Printer {
    void operator()(int i) const          { std::cout << "int: "    << i << "\n"; }
    void operator()(double d) const       { std::cout << "double: " << d << "\n"; }
    void operator()(const std::string& s) const { std::cout << "string: " << s << "\n"; }
};

int main() {
    Value v = 42;                       // 装 int，下标自动记为 0
    std::visit(Printer{}, v);           // 按当前下标分派到对应 operator()

    v = std::string("hello");           // 换装 string，下标自动更新
    std::visit(Printer{}, v);

    // 取值方式一：先问再取
    if (std::holds_alternative<std::string>(v))
        std::cout << std::get<std::string>(v) << "\n";

    // 取值方式二：get_if 返回指针，类型不对给 nullptr
    if (auto p = std::get_if<int>(&v))
        std::cout << *p << "\n";        // 本次不会进来

    std::cout << "当前类型下标: " << v.index() << "\n";   // 2
}
```

**为什么 `visit` 比 `get` / `holds_alternative` 更好？穷尽检查。** 用 `if (holds_alternative<...>)` 一路 else-if，本质还是手写 switch——哪天 variant 加了第四个类型 `bool`，所有手写分支都静默漏判。而 `visit` 要求 visitor **对每一个备选类型都可调用**，`Printer` 少一个 `operator()(bool)`，**编译直接失败**。「加新类型后忘了改某处分支」这个经典 bug，从「上线后才发现」提前到「编译期就拦下」——这正是本模块主线。

| | C 的 union + tag | `std::variant` |
|---|---|---|
| tag 维护 | 手动，会忘 | 编译器自动 |
| 读错成员 | UB，静默 | 抛异常 / 返回 nullptr |
| 装 string 等非平凡类型 | 不能 | 能（自动管构造析构） |
| 加新类型后的穷尽性 | 靠自觉逐处检查 | `visit` 编译期强制 |

适用场景：一个值「是有限几种之一」——解析结果是数字/字符串/布尔、状态机的几种状态各带不同数据。相比继承 + 虚函数（M3），它是值语义、无堆分配、类型集合封闭，小场景更轻。

## 3. `std::string_view`：`char* + len` 惯用法的类型化

### 3.1 问题：只读看一眼字符串，两头不讨好

写个只读函数，参数类型怎么选都别扭：

```cpp
int countSpaces(const std::string& s);   // 调用方传字面量 "a b" → 构造临时 string：堆分配+拷贝，就为看一眼
int countSpaces(const char* s);          // 调用方持有 string → 得 .c_str()；且丢了长度，内部还要 strlen
```

C 老手的解法你肯定写过：`f(const char* p, size_t len)`——指针加长度一起传。这个惯用法本身是对的，问题是它是**两个散装参数**：谁都可能把不配套的 p 和 len 传进来，每个函数签名都要重复一遍，还没法直接 `==` 比较、没有 `find`/`substr`。

### 3.2 机制：指针 + 长度，不拥有

`std::string_view` 的全部内容就是两个成员：

```
string_view 的布局：
+-----------+----------+
| 指针 ptr  | 长度 len  |     → sizeof = 16（64 位下）
+-----------+----------+
```

**它就是 C 的 `(char*, len)` 惯用法打包成一个类型**，附赠 `find`/`substr`/`==` 等操作。按值传参就是拷 16 字节，比传 `const std::string&`（解引用）还轻。`std::string`、字符串字面量、`char*` 都能无缝转成它——因为它们都能提供「指针 + 长度」。

关键推导：**它只有指针，没有那块字符内存——它不拥有数据**。由「不拥有」直接推出它的全部纪律：

- 被指向的数据死了，view 就是悬垂指针。**谁会死？** 局部 `string` 出作用域会死；**临时 `string` 在当前语句结束就死**。
- 所以：**只用它当函数参数**（调用期间实参必然活着），**不要用它存储**（成员变量、全局、返回值）——存储意味着 view 活得可能比数据久。

```cpp
// 反例：两种典型悬垂
std::string_view f() {
    std::string local = "temp";
    return local;                    // 返回后 local 析构，view 指向尸体
}
std::string_view sv = std::string("abc") + "def";  // 临时 string 本语句结束即亡，sv 当场悬垂
```

这和 M4 讲的悬垂指针是同一个问题——view 本质上就是带长度的指针，指针的生命周期纪律它一条不少。

### 3.3 用法

```cpp
#include <iostream>
#include <string>
#include <string_view>

// 只读参数首选 string_view，按值传
bool startsWith(std::string_view s, std::string_view prefix) {
    // string_view 的 substr 只是移动指针和长度，不拷贝任何字符
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

int main() {
    std::string name = "hello.wav";
    std::cout << startsWith(name, "hello") << "\n";   // 从 string 来：不拷贝
    std::cout << startsWith("data.txt", "data") << "\n"; // 从字面量来：不构造 string
    std::cout << sizeof(std::string_view) << "\n";    // 16：指针 + 长度，仅此而已
}
```

⚠️ 还有个 C 程序员专属坑：`string_view` **不保证 `\0` 结尾**（它可能是大字符串中间的一段切片），不能把 `sv.data()` 当 C 字符串传给 `printf("%s")` / `strlen`。要交给 C API，先 `std::string(sv)` 落地。

| | `const char*` | `const std::string&` | `std::string_view` |
|---|---|---|---|
| 携带长度 | 否（strlen） | 是 | 是 |
| 接字面量零拷贝 | 是 | 否（构造临时） | 是 |
| 接 std::string | 要 .c_str() | 是 | 是 |
| 拥有数据 | 否 | 是 | **否 → 只传参，不存储** |

## 4. 结构化绑定：`auto [a, b] = ...`

这个特性不是新类型，是给「多值访问」去掉约定负担：`.first`/`.second`、`it->second` 这种按位置访问，名字不表意，写错了编译器也不知道。结构化绑定让你**给每个成员当场起名**。

机制（够用即止）：`auto [a, b] = expr;` 脱糖成两步——编译器先生成一个**隐藏对象** `auto __e = expr;`（整体拷一次），然后 `a`、`b` 不是独立变量，而是**绑定到 `__e` 各成员的名字（别名）**。所以：

- `auto [a, b]`：隐藏对象是拷贝——改 a 不影响原对象；
- `auto& [a, b]`：隐藏引用绑定原对象——a、b 就是原对象成员的别名，能改原对象；
- `const auto& [a, b]`：只读不拷贝，**遍历容器的首选**。

```cpp
#include <iostream>
#include <map>
#include <string>
#include <utility>

std::pair<bool, int> parseDigit(char c) {
    if (c >= '0' && c <= '9') return {true, c - '0'};
    return {false, 0};
}

int main() {
    // 最高频场景：遍历 map。比 it->first / it->second 可读性高一档
    std::map<std::string, int> scores{{"Alice", 90}, {"Bob", 85}};
    for (const auto& [name, score] : scores)
        std::cout << name << ": " << score << "\n";

    // 拆 pair 返回值
    auto [ok, val] = parseDigit('7');
    if (ok) std::cout << "digit = " << val << "\n";

    // 拆结构体：按成员声明顺序绑定
    struct Point { int x, y; };
    Point p{3, 4};
    auto [px, py] = p;
    std::cout << px << "," << py << "\n";
}
```

绑定个数必须和成员数**完全一致**，多一个少一个都编译错误——结构体加了字段，所有拆它的地方被编译器点名，这又是「变更被编译器追踪」而不是「靠人排查」。

## 5. `constexpr` 与 `if constexpr`：把计算搬进编译期

### 5.1 问题：C 的编译期常量只有宏，三宗罪

C 里想要「编译期定死的值」基本只有 `#define`：

```c
#define MAX_SIZE 100
#define SQUARE(x) ((x)*(x))
```

三宗罪：**没类型**（就是文本替换，塞哪儿都行，错了报奇怪的错）；**没作用域**（污染整个翻译单元，还能被 `#undef` 暗算）；**函数式宏重复求值**（`SQUARE(i++)` 展开成 `((i++)*(i++))`，M6 讲模板时骂过）。根源相同：预处理器根本不懂 C 语言，它只是个文本替换器，编译器看到的是替换后的残局。

`const` 能救一半——有类型有作用域——但 `const int n = f();` 的值可能运行期才定，C++17 前还不一定能当数组大小。缺一个明确说「这个值编译期必须可知」的工具。

### 5.2 机制：编译器内跑一个解释器

`constexpr` 标在变量上，意思是「此值编译期必须算得出来，算不出来就报错」。标在函数上，意思是「这个函数**有资格**在编译期被调用」。

编译期求值发生在哪？直觉模型：**编译器内部带了一个 C++ 解释器**。编译到 `constexpr int f5 = factorial(5);` 时，编译器当场解释执行 `factorial` 的函数体——循环真的循环、变量真的赋值——算出 120，然后把 120 当作字面量嵌进生成的代码。运行期没有任何调用发生。

`constexpr` 函数是**两栖**的：

- 实参是编译期常量、且结果被用在需要常量的地方（数组大小、`static_assert`、模板参数、`constexpr` 变量初始化）→ 编译期算；
- 实参是运行期值 → 自动退化成普通函数，运行期照常调用。

一份代码两种用法，不用像 C 那样「宏一份、函数一份」维护两套。

```cpp
#include <iostream>

constexpr long long factorial(int n) {   // 有资格编译期执行的普通函数
    long long r = 1;
    for (int i = 2; i <= n; ++i) r *= i;
    return r;
}

int main() {
    constexpr long long f5 = factorial(5);      // 编译期算出 120
    static_assert(f5 == 120, "编译期就能验证");   // 编译期断言，运行期零开销

    int arr[factorial(4)] = {};                 // 能当数组大小 → 证明是真·编译期常量
    std::cout << sizeof(arr) / sizeof(arr[0]) << "\n";   // 24

    int n = 0;
    std::cin >> n;
    std::cout << factorial(n) << "\n";          // 运行期值 → 退化为普通函数调用
}
```

| | `#define` 宏 | `const` | `constexpr` |
|---|---|---|---|
| 类型检查 / 作用域 | 无 / 无 | 有 / 有 | 有 / 有 |
| 保证编译期可求 | 是（文本） | 不保证 | **保证** |
| 函数版本 | 重复求值坑 | — | 两栖，无坑 |
| 调试器可见 | 否 | 是 | 是 |

### 5.3 `if constexpr`：假分支根本不实例化

普通 `if` 是运行期选择，**两个分支都必须编译通过**。`if constexpr` 是编译期选择，**条件为假的分支直接被丢弃，不参与该模板实例的编译**。区别在模板里是生死攸关的：

```cpp
#include <iostream>
#include <string>
#include <type_traits>

template <typename T>
void describe(const T& v) {
    if constexpr (std::is_pointer_v<T>) {
        std::cout << "指针，指向: " << *v << "\n";   // 只有 T 是指针时这行才会被编译
    } else if constexpr (std::is_integral_v<T>) {
        std::cout << "整数: " << v << "\n";
    } else {
        std::cout << "其他: " << v << "\n";
    }
}

int main() {
    int x = 42;
    describe(x);                    // 走"整数"分支；*v 那行对 int 非法，但根本没编译它
    describe(&x);                   // 走"指针"分支
    describe(std::string("hi"));    // 走"其他"分支
}
```

如果第一处用普通 `if`：`describe(x)` 实例化时 `*v` 对 `int` 非法，**编译失败**——哪怕运行期永远走不到那行。`if constexpr` 让「按类型走不同代码」一行搞定，取代了 C++17 之前的 SFINAE 黑魔法。这呼应 M6 的按需实例化：模板只为用到的部分生成代码，`if constexpr` 把粒度细化到了分支级。

## 6. `enum class`：底层还是整数，但类型系统站岗

### 6.1 问题：C 的 enum 就是穿了马甲的 int

```c
enum Color { RED, GREEN, BLUE };

int x = RED;             // 罪一：随意隐式转 int，编译器不拦
if (RED == 0) {}         // 和任意整数比较也不拦，写错枚举比错类型都发现不了
// 罪二：RED、GREEN 直接暴露在外层作用域，另一个 enum 再想用 RED 就撞名
// 罪三：底层类型由编译器定，跨平台大小不确定，没法安全地放进文件格式/网络协议
```

C 的 enum 只是「给整数起了几个名字」，类型系统上它和 int 几乎无隔离——约定「这个变量只取这几个值」，但谁都能塞任意 int 进去比较、运算。

### 6.2 机制与用法

`enum class` 的底层**仍然是一个整数**（这点没变，不神秘），变的是类型系统的态度：**隐式转换被挡死，枚举名锁进自己的作用域**。要转整数必须 `static_cast`，「我明确知道我在拿底层值」变成代码里可见的动作。还能显式指定底层类型，大小从此可控：

```cpp
#include <cstdint>
#include <iostream>
#include <string>

// 指定底层类型 uint16_t：大小确定，可安全用于文件格式字段（WAV 头里就有这样的字段）
enum class Format : std::uint16_t { Pcm = 1, Float = 3 };
enum class Status { Ok, Error };   // 不指定则默认 int

std::string toString(Format f) {
    switch (f) {                   // switch 漏了枚举值，/W4 或 -Wall 会警告——又是编译器替你查岗
        case Format::Pcm:   return "PCM";
        case Format::Float: return "IEEE float";
    }
    return "unknown";
}

int main() {
    Format f = Format::Pcm;        // 必须带作用域 Format::，不污染外层
    // int x = f;                  // 反例：编译错误，不隐式转 int
    int x = static_cast<int>(f);   // 要底层值？显式说出来
    std::cout << toString(f) << " " << x << "\n";
    std::cout << (Status::Ok == Status::Error) << "\n";  // 只能和同类型比
    // if (f == Status::Ok) {}     // 反例：编译错误，跨枚举比较被挡
}
```

新代码一律 `enum class`。它是「受限整数」这个约定的类型化：值域受限、比较受限、转换显式、大小可指定。

## 7. 属性（attributes）：把注释里的约定变成编译器能读的标注

你在 C 代码里一定写过这类注释：`/* 调用者必须检查返回值 */`、`/* 此参数 debug 版才用 */`、`/* 此处故意 fall through */`。注释编译器读不懂，约定照样会被忘。C++11 起的 `[[...]]` 属性把这几条最常见的注释标准化成**编译器可检查的标注**：

```cpp
#include <iostream>

[[nodiscard]] bool tryConnect() { return false; }   // "返回值不许忽略"——从注释变成警告

[[deprecated("请改用 newApi()")]] void oldApi() {}  // 调用处直接出警告，附迁移提示
void newApi() {}

void log(int level, [[maybe_unused]] int verbose) { // "此参数可能没用到"，压掉 -Wunused 告警
    switch (level) {
        case 2:
            std::cout << "详细信息\n";
            [[fallthrough]];       // "我故意不 break"——不写它，/W4 会怀疑你漏了
        case 1:
            std::cout << "基本信息\n";
            break;
    }
}

int main() {
    tryConnect();                  // 反例：警告——nodiscard 返回值被丢弃
    if (!tryConnect()) std::cout << "连接失败\n";   // 正确姿势
    log(2, 0);
}
```

最值得养成习惯的是 `[[nodiscard]]`：标在「返回值就是全部意义」的函数上（错误码、`try_lock` 的成败、工厂函数返回的对象）。C 里「忘了检查 `fopen` 返回值」这类 bug，就是缺这个机制。属性不改变程序语义，只影响诊断——但「多一条编译警告」正是本模块主线里最便宜的一种保证。

---

# 第二部分：错误处理——把「记得检查」变成「不可忽略的控制流」

## 8. 异常机制：throw 之后到底发生了什么

### 8.1 问题：错误码模式的结构性缺陷

C 报告错误只有返回值 / errno 一条路，缺陷是结构性的：

```c
int loadConfig(Config* out) {
    FILE* fp = fopen("cfg.txt", "r");
    if (!fp) return -1;                    // 检查一次
    char* buf = malloc(SIZE);
    if (!buf) { fclose(fp); return -2; }   // 检查一次 + 手动清理已拿到的资源
    if (parse(fp, buf, out) != 0) { free(buf); fclose(fp); return -3; }  // 再来
    free(buf); fclose(fp);
    return 0;
}
```

三条硬伤：**错误可以被忽略**——调用方不写 `if (loadConfig(...) != 0)`，编译照过，错误静默蒸发；**中间层被迫参与**——错误发生在第 5 层、处理在第 1 层，中间 3 层每层都得「检查 → 清理 → 转发」，忘一层就断链；**构造函数没有返回值**——对象构造失败根本无处报告，只能搞「二段式初始化」（构造 + `init()`），而那正是 M2 批判过的反模式。

**异常把「错误传播」从人肉接力变成语言机制**：出错处 `throw`，能处理处 `catch`，中间层一行错误代码都不用写，而且**想忽略都忽略不了**——没人接住就 `terminate`，错误绝不静默蒸发。

### 8.2 机制：throw 之后的三步

`throw` 一执行，运行时做三件事，每一步都值得看清：

**第一步：构造异常对象。** `throw std::runtime_error("boom")` 把抛出的值拷贝/移动到一块**由运行时管理的特殊存储**里。为什么不放在当前栈帧？因为当前栈帧马上要被拆掉，异常对象必须活到 catch 处理完为止。

**第二步：栈展开（stack unwinding）。** 运行时沿调用链逐帧往上走，每退出一帧，**这一帧里已构造完成的局部对象按构造的逆序依次析构**——和函数正常 return 时的清理完全一样，只是触发原因不同。这一步是理解 C++ 错误处理的钥匙，下一节展开。

**第三步：匹配 catch。** 每到一帧，看它有没有 `try` 块、catch 的类型是否匹配（支持按基类接住派生类）。找到第一个匹配的 catch 就停下，把异常对象交给它。一路到 `main` 都没人接？调用 `std::terminate`，程序当场结束——这就是「不可忽略」的兜底。

```cpp
#include <iostream>
#include <stdexcept>
#include <string>

struct Guard {                       // 一个最小 RAII 类，用来观察栈展开
    std::string name;
    explicit Guard(std::string n) : name(std::move(n)) { std::cout << "获取: " << name << "\n"; }
    ~Guard() { std::cout << "释放: " << name << "\n"; }
};

void inner() {
    Guard g("inner 的资源");
    throw std::runtime_error("inner 出错");   // 从这里开始栈展开
}

void middle() {
    Guard g("middle 的资源");
    inner();                          // 注意：middle 没写任何错误处理代码
    std::cout << "这行不会执行\n";
}

int main() {
    try {
        middle();
    } catch (const std::exception& e) {       // 按 const 引用接住
        std::cout << "捕获: " << e.what() << "\n";
    }
}
// 输出顺序（构造顺序进、逆序析构出）：
// 获取: middle 的资源 → 获取: inner 的资源 → 释放: inner 的资源 → 释放: middle 的资源 → 捕获: inner 出错
// 两个 Guard 都被释放了——middle 一行清理代码没写
```

要点：
- **catch 永远按 `const` 引用**（`const std::exception&`）：按值捕获会把派生类异常切片成基类（M3 的对象切片），丢掉真实信息，还多拷一次。
- 标准异常都派生自 `std::exception`，`what()` 给描述；自定义异常也应继承它（通常继承 `std::runtime_error`），这样一个 `catch (const std::exception&)` 能兜住全家。
- `catch (...)` 接住一切但拿不到信息，只适合「最外层记日志再重抛」。
- MSVC 编译带异常的代码必须开 `/EHsc`（我们的标准命令行一直带着它，现在你知道它是干嘛的了）。

### 8.3 vs C 的对比

| | C 错误码 / errno | C++ 异常 |
|---|---|---|
| 能否被忽略 | 能（忘了 if 就静默） | 不能（没人接就 terminate） |
| 中间层负担 | 每层检查+清理+转发 | 零（栈展开自动清理） |
| 构造函数报错 | 无解（二段式初始化） | 天然支持 |
| 错误信息 | 一个 int | 任意类型的对象，可带上下文 |
| 正常路径开销 | 每次调用都要检查 | 不抛时近乎零 |

## 9. RAII 与异常安全：栈展开为什么保证资源不漏

### 9.1 第三条离开路径

M4 讲 RAII 时说过：资源在构造函数里获取、析构函数里释放，函数无论从哪条路径离开，局部对象的析构都会执行。当时数了两条路径——正常走到底、中途 return。**异常是第三条路径**：栈展开时，每一帧里已构造完成的局部对象同样保证析构。

这就是上面示例里 `middle` 一行清理代码都没写、资源却全部释放的原因。对比 8.1 的 C 版本 `loadConfig`：每个出错分支手动 `fclose`/`free`，三个分支三份清理代码，漏一处就泄漏。C++ 版本里 `fstream`、`vector`、`unique_ptr`、`lock_guard` 全是 RAII 对象，**清理代码不存在，所以不可能写漏**——异常安全不是「小心翼翼地在每个 catch 里清理」，而是「让清理自动发生」。

**推论（重要）：异常这套机制的可靠性完全建立在 RAII 上。** 如果你在 C++ 里写 `char* buf = new char[n]; ... delete[] buf;` 这种裸资源代码，中间抛异常 `delete[]` 就被跳过——异常反而成了泄漏放大器。这就是 M4 说「资源必须住在对象里」的深层原因：不是风格洁癖，是异常机制的前提。

### 9.2 异常安全三级别（面试高频）

「这个函数是异常安全的」太模糊，工程上分三档承诺，一档比一档强：

| 级别 | 承诺 | 直白版 | 例子 |
|---|---|---|---|
| **基本保证** | 不泄漏资源，对象仍处于合法（但可能已变的）状态 | 「出错了东西没坏，但可能不是原样了」 | 大多数正确使用 RAII 的函数天然达到 |
| **强保证** | 要么完全成功，要么回到调用前的状态 | 「事务：失败等于没发生」 | `vector::push_back`（扩容失败时原 vector 不变） |
| **不抛保证** | 绝不抛异常 | 「我一定成功」 | 析构、swap、移动操作（应当如此） |

- **基本保证**是底线：只要资源全在 RAII 对象里、不留半初始化状态，基本就有。达不到基本保证的代码（出错后对象状态非法/资源泄漏）是 bug。
- **强保证**的经典实现是 **copy-and-swap**（M5 见过）：先在副本上做完所有可能失败的操作，全成功后用 `noexcept` 的 swap 一步换入。可能失败的步骤都发生在副本上，原对象要么被整体替换、要么原封不动。强保证有代价（副本），不必处处追求。
- **不抛保证**是给别人搭积木用的地基：swap 不抛，copy-and-swap 才成立；移动不抛，vector 扩容才敢用移动（见下节）。

### 9.3 `noexcept`：承诺、违约与它撬动的优化

`noexcept` 是函数对外的**承诺**：「我不抛异常」。它不是「编译器帮你保证不抛」，而是**违约后果自负**：`noexcept` 函数里真的有异常跑出来，不做栈展开，直接 `std::terminate`。所以它是一份写进类型系统的军令状。

为什么要立这个军令状？因为**调用方能拿这个承诺做重要决策**。最经典的就是 M5 埋过伏笔的 vector 扩容：

- 扩容 = 分配新内存 + 把旧元素逐个搬到新内存。用移动搬最快。
- 但移动是**破坏性**的：搬到第 500 个时抛了异常，前 500 个已被搬空、后面的还在原地——旧 vector 回不去了，强保证碎了。拷贝没这个问题：源始终完好，出错扔掉新内存即可回滚。
- 所以 vector 的策略是（`std::move_if_noexcept`）：**元素的移动构造标了 `noexcept` 才敢用移动，否则退回拷贝**。你的类不标，装进 vector 就默默走慢速路径。

```cpp
#include <iostream>
#include <string>
#include <vector>

struct Elem {
    std::string data;
    Elem() = default;
    Elem(const Elem& o) : data(o.data) { std::cout << "拷贝\n"; }
    Elem(Elem&& o) noexcept : data(std::move(o.data)) { std::cout << "移动\n"; }
    // ↑ 去掉 noexcept，扩容时打印的就全是"拷贝"——vector 不敢用不做承诺的移动
};

int main() {
    std::vector<Elem> v;
    v.reserve(1);
    v.emplace_back();
    v.emplace_back();   // 触发扩容：搬第一个元素时打印"移动"
}
```

三条纪律：**移动构造/移动赋值尽量 `noexcept`**（M5 的忠告在这里兑现）；**析构函数默认就是 noexcept，绝不让异常逃出析构**（栈展开中再抛一个 = 双异常 = terminate）；**swap 应当 noexcept**（强保证的地基）。

## 10. 异常 vs 错误码：选型判断

异常不是错误码的替代品，两者分工。判断的核心问题是：**这个失败是「预期内的正常结果」还是「异常情况」？**

| 维度 | 异常 | 错误码 / `optional` |
|---|---|---|
| 语义 | 罕见、破坏性、就地处理不了 | 预期内的失败：查不到、输入不合法 |
| 正常路径开销 | 近乎零（不抛不花钱） | 每次调用都要检查分支 |
| 失败路径开销 | 大（构造异常对象 + 栈展开） | 无额外开销 |
| 构造函数 | 唯一选择 | 用不了（没返回值） |
| 热路径 / 跨二进制边界 / 回调进 C | 慎用 | 首选 |

经验法则：

- **「没查到」「用户输入非法」「文件里这个块是可选的」**——这是程序的正常分支，用 `optional` / 错误码。拿异常当这种控制流，又慢又乱。
- **构造函数失败、系统资源耗尽、违反前置条件、深层遇到无法继续的状况**——用异常，让它一路穿透到有能力处理的层。
- 判据反过来用也成立：如果你发现调用方**每次调用都要 try/catch**，说明这个失败是预期内的，改 `optional`；如果你发现错误码**层层原样转发从没人处理**，说明它该是异常。

> C++23 有 `std::expected<T, E>`（要么值要么错误原因，类似 Rust 的 `Result`）。C++17 下可用 `std::optional`（不需要原因）或 `std::variant<T, Error>`（需要原因）顶替。

---

# 第三部分：并发——把「小心共享数据」变成结构性保证

> 并发的水极深（内存序、无锁结构），本模块建立够用且**正确**的心智模型：怎么开线程、数据竞争到底是什么、锁和原子怎么选、等待怎么不空转。这四样撑起 mini 项目（线程安全队列），也是面试并发题的主干。

## 11. `std::thread`：线程的 RAII 化身（半个）

C 里开线程用平台 API（POSIX 的 `pthread_create` / Windows 的 `CreateThread`），两套代码不通用；C++11 起标准库统一为 `std::thread`，跨平台同一份代码。

```cpp
#include <iostream>
#include <thread>

void work(int id, int rounds) {
    long sum = 0;
    for (int i = 0; i < rounds; ++i) sum += i;
    std::cout << "线程 " << id << " 算完: " << sum << "\n";   // 输出可能交错，正常
}

int main() {
    std::thread t1(work, 1, 1000);   // 构造即启动：work(1, 1000) 在新线程跑
    std::thread t2(work, 2, 1000);
    std::cout << "主线程继续干别的\n";
    t1.join();                       // 阻塞等 t1 结束
    t2.join();
}
```

要点：

- **析构前必须 `join()` 或 `detach()` 二选一，否则析构直接 `std::terminate`。** 这个设计很「本模块」：一个还在跑的线程被无声抛弃，几乎必然是 bug（它可能还在用你的栈变量），标准库拒绝替你猜，宁可炸出来逼你表态。
- `join()`：等它跑完。要结果、要顺序，用它。`detach()`：放生，线程后台自跑。慎用——detach 出去的线程访问了已析构的对象就是 UB，且没人知道它什么时候结束。日常规则：**能 join 就 join**。
- 参数默认**拷贝**进新线程（哪怕函数签名是引用）——这是防悬垂的保守设计。真要共享，显式 `std::ref(x)`，此时生命周期归你管。

## 12. 数据竞争与 `std::mutex`：从 `++counter` 推演起

### 12.1 一步步看丢失的更新

`++counter` 在源码里是一行，在机器上是**三步**：读内存到寄存器、寄存器加一、写回内存。单线程时这三步永远连续；两个线程同时跑，三步会**交错**。设两个线程各执行一次 `++counter`，counter 初值 5：

| 时刻 | 线程 A | 线程 B | 内存里的 counter |
|---|---|---|---|
| 1 | 读 counter → 寄存器 a=5 | | 5 |
| 2 | | 读 counter → 寄存器 b=5 | 5 |
| 3 | a = a+1 → 6 | | 5 |
| 4 | | b = b+1 → 6 | 5 |
| 5 | 写回 6 | | 6 |
| 6 | | 写回 6 | **6（不是 7！）** |

两次自增，counter 只前进了一格——B 的写回**覆盖**了 A 的成果，一次更新凭空蒸发。两个线程各自增 1000 次，任何一次交错都可能丢一格，最终结果落在 1000~2000 之间的随机位置，且每次运行都不同。这就是**数据竞争**的教科书样本：两个以上线程访问同一内存、至少一个在写、没有同步。

### 12.2 为什么是 UB，而不只是「结果不准」

如果数据竞争只是「丢几次更新」，那顶多算结果不准。但标准把它定义为**未定义行为**，比不准严重得多，原因在编译器：**编译器优化的全部前提是「没有别的线程在看这块内存」**。基于这个前提它可以：把 `counter` 整个循环期间缓存在寄存器里，最后写回一次（另一线程的更新全程不可见）；把 `for (...) ++counter;` 直接折叠成 `counter += 100000;`；把读写重排到它认为等价的位置。单线程下这些优化全部正确，一旦有竞争，程序行为和源码可以毫无对应关系——不是「慢半拍的旧值」，是「任何事都可能发生」。所以规矩是硬的：**共享可变数据，必须同步，没有侥幸。**

### 12.3 `std::mutex` + `lock_guard`：RAII 主线在锁上兑现

`std::mutex` 是互斥区的**入场券**：`lock()` 拿票进场（票被别人拿着就排队睡等），`unlock()` 交票离场，任何时刻场内至多一人。三步读-改-写在场内完成，别的线程连第一步都开始不了，交错被结构性排除。

但手动 `lock()`/`unlock()` 是 C 风格的纪律活：中途 `return` 忘了 unlock、**中途抛异常跳过 unlock**——票永远不归还，所有排队线程永久睡死（死锁）。听着耳熟吗？这就是 M4 里「手动 free 会漏」的锁版本。解法也是同一个：**把 unlock 绑进析构函数**——

```cpp
#include <iostream>
#include <mutex>
#include <thread>

std::mutex mtx;
long counter = 0;

void addN(int n) {
    for (int i = 0; i < n; ++i) {
        std::lock_guard<std::mutex> lk(mtx);   // 构造 = lock，拿票进场
        ++counter;                             // 临界区：同一时刻只有一个线程在这
    }                                          // 析构 = unlock —— return、异常，条条路径都归还
}

int main() {
    std::thread t1(addN, 100000);
    std::thread t2(addN, 100000);
    t1.join();
    t2.join();
    std::cout << counter << "\n";   // 恰好 200000，每次都是
}
```

`lock_guard` 就是 RAII 应用在锁上：M4 管内存、管文件，这里管锁，同一个思想——**「释放」写在析构函数里，就再也不存在「忘了释放」这条路径，异常路径也天然覆盖**（第二部分的栈展开在这儿直接服务并发正确性，两条线在此汇合）。

再认识一个亲戚：`std::unique_lock`——功能超集，能中途 `unlock()` 再 `lock()`、能延迟上锁、能转移所有权，代价是略重。日常首选 `lock_guard`，**用 `condition_variable` 时必须 `unique_lock`**（第 14 节说为什么）。多把锁一起上用 C++17 的 `std::scoped_lock`（内部算法保证不死锁）。

## 13. `std::atomic`：单条不可分割的硬件指令

### 13.1 机制

回看 12.1 的推演，问题出在「三步之间可以插队」。锁的思路是把三步圈进互斥区；`std::atomic` 的思路更釜底抽薪：**让硬件把这三步合成一条不可分割的指令**。`std::atomic<long>` 的 `++`，在 x86 上编译成一条 `lock xadd`——CPU 保证这条指令的读-改-写期间，其他核对这个地址的访问被排开（缓存一致性协议仲裁）。没有「三步之间」，就没有插队。

代价对比：原子指令比普通加法慢（多核要争缓存行），但比 mutex 便宜一个量级——mutex 抢不到票要进内核睡眠、上下文切换；原子操作永远不睡，硬件层面直接完成。

```cpp
#include <atomic>
#include <iostream>
#include <thread>

std::atomic<long> counter{0};      // 不需要 mutex

void addN(int n) {
    for (int i = 0; i < n; ++i)
        ++counter;                 // 单条原子指令，无竞争、无 UB
}

int main() {
    std::thread t1(addN, 100000);
    std::thread t2(addN, 100000);
    t1.join();
    t2.join();
    std::cout << counter.load() << "\n";   // 恰好 200000
}
```

### 13.2 边界：atomic 保护「一次操作」，保护不了「不变量」

atomic 的保证严格限于**单个变量上的单次操作**。多个操作、多个变量之间的一致性（复合不变量），它无能为力：

```cpp
// 反例：check-then-act —— 两次原子操作，合起来不原子
std::atomic<int> balance{100};
void withdraw(int amount) {
    if (balance >= amount)         // 原子地读了一次 ✓
        balance -= amount;         // 原子地减了一次 ✓
}   // 但两步之间别的线程可能已把钱取走 —— 余额照样能被扣成负数
```

两个线程同时通过 `if`（各自读到 100），再先后各扣 80——余额 -60。每一步都原子，**组合不原子**。同理「队列 + 计数器保持一致」「先查再插」这类跨变量、跨步骤的不变量，都必须回到 mutex 把整段圈起来。

选型口诀：**单变量的独立计数/标志位 → atomic；一段逻辑或多个变量的一致性 → mutex。** 最常见的 atomic 用法就两种：计数器（如上）、停止标志 `std::atomic<bool> stop{false};`（一个线程置位，其他线程轮询退出）。

| | `std::atomic` | `std::mutex` |
|---|---|---|
| 机制 | 单条不可分割硬件指令 | 入场券，挡在临界区外 |
| 保护范围 | 单变量的单次操作 | 任意长的代码段、任意多的变量 |
| 开销 | 小（不睡眠） | 较大（争用时睡眠+切换） |
| 复合不变量 | **不能** | 能 |

## 14. `std::condition_variable`：等待-通知，以及为什么必须谓词循环

### 14.1 问题：怎么「等到队列里有东西」

消费者线程要等队列非空。不用工具的话只能忙轮询：

```cpp
// 反例：忙等 —— 空转烧 CPU，还得反复抢锁骚扰生产者
while (true) {
    std::lock_guard<std::mutex> lk(mtx);
    if (!q.empty()) break;
}
```

`condition_variable` 提供**等待-通知**模型：等的人**睡过去**（不占 CPU），条件可能成立时由制造条件的人**叫醒**它。核心两个动作：`cv.wait(lk)` 睡下；`cv.notify_one()` / `notify_all()` 叫醒一个/全部。

`wait(lk)` 内部干的事必须看清（这解释了两条规矩）：**原子地「解锁 + 睡下」**（不解锁的话生产者永远拿不到锁、永远没法生产，谁也叫不醒你）；被叫醒后，**先重新抢回锁，再从 wait 返回**。因为 wait 要能中途解锁/加锁，搭配的必须是灵活的 `unique_lock`，`lock_guard` 干不了。

### 14.2 为什么必须「谓词循环」等待

规矩：**永远用带谓词的 wait**——`cv.wait(lk, []{ return !q.empty(); })`，它等价于：

```cpp
while (!q.empty() == false)   // 即：条件不满足就继续睡
    cv.wait(lk);
```

为什么醒来不能直接干活、必须回头再查一遍条件？两个独立的理由，推演给你看：

**理由一：虚假唤醒。** 操作系统允许 wait 在**没人 notify 的情况下偶尔自己醒来**（底层实现权衡的产物，各平台都有）。醒了 ≠ 有货，不查就取，空队列上 `front()` 直接 UB。

**理由二（更根本）：从被唤醒到抢回锁之间，世界可能已经变了。** 推演：队列空，消费者 A、B 都在 wait 里睡。生产者 push 一个元素，`notify_all` 叫醒 A 和 B。两人都要先抢锁才能从 wait 返回——A 先抢到，取走唯一的元素，解锁走人；**B 这才抢到锁，从 wait 返回，而队列已经又空了**。B 收到的通知是真的（当时确实有货），但「醒来」和「拿到锁」之间隔着一段别人可以行动的窗口。所以醒来后**拿着锁重新检查条件**是唯一可靠的做法，谓词循环正是这个逻辑的固化。

### 14.3 生产者-消费者最小可运行版（mini 项目的骨架）

```cpp
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

std::mutex mtx;
std::condition_variable cv;
std::queue<int> q;
bool done = false;             // "不再生产了"——受 mtx 保护

void producer() {
    for (int i = 0; i < 10; ++i) {
        {
            std::lock_guard<std::mutex> lk(mtx);
            q.push(i);
        }                      // 先解锁再 notify：被唤醒者不用醒来就撞锁
        cv.notify_one();
    }
    {
        std::lock_guard<std::mutex> lk(mtx);
        done = true;           // 收工信号
    }
    cv.notify_all();           // 叫醒所有还睡着的消费者来看信号
}

void consumer(int id) {
    while (true) {
        std::unique_lock<std::mutex> lk(mtx);          // wait 需要 unique_lock
        cv.wait(lk, [] { return !q.empty() || done; }); // 谓词循环：有货或收工才往下走
        if (!q.empty()) {
            int item = q.front();
            q.pop();
            lk.unlock();       // 拿到货就出临界区，处理不占锁
            std::cout << "消费者 " << id << " 处理 " << item << "\n";
        } else {               // 队列空 + done：优雅退出
            return;
        }
    }
}

int main() {
    std::thread p(producer);
    std::thread c1(consumer, 1), c2(consumer, 2);
    p.join(); c1.join(); c2.join();
}
```

三处设计都为 mini 项目铺路：**谓词写 `!q.empty() || done`**（有货干活，没货但收工也要醒来退出，否则生产者结束后消费者睡死）；**退出判断放在拿锁之后**（判断 done 和 q 也是共享状态访问）；**取到数据就 unlock 再处理**（临界区只圈必要动作，锁粒度小并发度高）。mini 项目就是把这套骨架封装成 `ThreadSafeQueue<T>` 模板类——push 加锁入队再 notify，waitAndPop 谓词等待。

vs C：这一整套（thread/mutex/cv）在 C 里对应 pthread 的 `pthread_create`/`pthread_mutex_t`/`pthread_cond_t`，概念一一对应，但 C 版全程手动 lock/unlock、手动 while 循环防虚假唤醒——C++ 版把这些纪律分别固化进了 `lock_guard` 和带谓词的 `wait`。

---

# 第四部分：工程化——把「我这儿能编译」变成可复现的保证

前三部分是语言层面的保证，这一部分是**工具链层面**的：你的项目能不能在任何一台机器上一条命令构建（CMake）、每次改动有没有自动验证（测试 + CTest）、内存错误和坏味道有没有人自动盯着（Sanitizer、clang-tidy）。「在我机器上是好的」不是保证，可复现的流程才是。

## 15. CMake：描述目标与依赖的声明式清单

### 15.1 心智模型

单文件时代 `cl main.cpp` 就够了。文件一多、要链库、要跨平台（你的 MSVC、同事的 g++、CI 的 clang），手敲命令和手写 VS 工程都维护不动。

CMake 的心智模型一句话：**你写一份声明式清单（`CMakeLists.txt`），描述「有哪些构建目标、每个目标由哪些源文件构成、目标之间怎么依赖」；CMake 读这份清单，生成你本地平台的构建系统**（Windows 上生成 VS 工程，Linux 上生成 Makefile/Ninja）。它自己不编译任何东西——它是构建系统的**生成器**，清单是跨平台的，生成的东西是本地的。

### 15.2 最小 CMakeLists.txt，逐行讲

```cmake
cmake_minimum_required(VERSION 3.15)    # 本清单用到的 CMake 特性需要的最低版本
project(MyApp LANGUAGES CXX)            # 项目名 + 语言（CXX = C++），一个清单开头必写的两行

set(CMAKE_CXX_STANDARD 17)              # 目标用 C++17 编译
set(CMAKE_CXX_STANDARD_REQUIRED ON)     # 编译器不支持 17 就报错，而不是静默降级
set(CMAKE_CXX_EXTENSIONS OFF)           # 关编译器私有扩展，写的是标准 C++

add_executable(myapp main.cpp)          # 声明一个可执行目标 myapp，由 main.cpp 构成
```

多文件、分库（更接近真实工程的样子）：

```cmake
add_library(core tsqueue_helpers.cpp wav_utils.cpp)   # 声明一个库目标
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE core)             # 声明依赖关系：myapp 用到 core

find_package(Threads REQUIRED)                        # 找线程库（跨平台）
target_link_libraries(myapp PRIVATE Threads::Threads) # 用 std::thread 的目标要链它
```

注意整份清单没有出现任何编译器命令、任何平台路径——**只有目标和关系**。「怎么把关系变成 cl.exe 或 g++ 的具体命令行」是 CMake 生成阶段的事，和你无关了。

### 15.3 configure / build 两步流程

```bash
cmake -S . -B build      # 第一步 configure：读清单，在 build/ 里生成本地构建系统
cmake --build build      # 第二步 build：驱动生成的构建系统真正编译
```

`-S .` 源码目录（放 CMakeLists.txt 的地方），`-B build` 构建目录。所有生成物都进 `build/`，源码目录始终干净（out-of-source build，`build/` 加进 `.gitignore` 即可）。日常循环：改了 `.cpp` 只需重跑第二步；改了 `CMakeLists.txt` 也只需重跑第二步（它会发现清单变了、自动重新 configure）。

### 15.4 在 Visual Studio 里

VS 2019+ 原生支持 CMake：**「打开文件夹」选中含 `CMakeLists.txt` 的目录**即可——VS 自动 configure，目标出现在启动项下拉框里，F5 构建调试，不需要也不会生成 `.sln`/`.vcxproj`。命令行党也可以 `cmake -G "Visual Studio 17 2022" -S . -B build` 显式生成 VS 工程再打开。

## 16. 单元测试：断言 + 非零退出码，就这么多

### 16.1 测试为什么必须独立、可自动运行

「我测过了」的问题和「我记得 free 了」一样：靠人。手动测试改一次代码就作废一次，没人会每次改动全部重测。单元测试的本质是**把「验证」写成代码**：给函数已知输入、断言预期输出，改完代码一条命令重跑全部——回归（改 A 坏了 B）当场现形。要做到「一条命令、机器可判读」，只需要一个约定：**测试程序全过退出码为 0，有失败退出码非 0**。构建系统和 CI 只看退出码。

### 16.2 零依赖断言宏，逐行讲

不引任何框架，一个宏起步：

```cpp
#include <iostream>

static int g_failures = 0;                 // 累计失败数

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::cerr << "FAIL: " << #cond << " ("                      \
                      << __FILE__ << ":" << __LINE__ << ")\n";          \
            ++g_failures;                                               \
        }                                                               \
    } while (0)
// #cond：预处理器把条件表达式原文变成字符串，报错时能看见"哪个断言挂了"
// __FILE__/__LINE__：出错位置；do{}while(0)：让宏在 if/else 里表现得像一条语句（C 宏的老惯用法）
// 失败不退出、只计数：一次跑完看到全部失败，而不是挂一个停一个

int clampInt(int v, int lo, int hi) {      // 被测函数
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int main() {
    CHECK(clampInt(5, 0, 10) == 5);        // 正常值
    CHECK(clampInt(-3, 0, 10) == 0);       // 下越界
    CHECK(clampInt(99, 0, 10) == 10);      // 上越界
    CHECK(clampInt(0, 0, 10) == 0);        // 边界本身
    if (g_failures == 0) { std::cout << "全部通过\n"; return 0; }
    std::cerr << g_failures << " 个失败\n";
    return 1;                              // 非零退出码：机器可判读的"失败"
}
```

### 16.3 接入 CTest

CMake 自带测试驱动 CTest，把测试程序注册进去，就获得「一条命令跑全部测试」：

```cmake
enable_testing()
add_executable(tests tests.cpp)
add_test(NAME unit_tests COMMAND tests)   # 约定生效处：退出码非零 = 测试失败
```

```bash
ctest --test-dir build       # 跑全部注册的测试，汇总通过/失败
```

真实项目用成熟框架——**GoogleTest**、**Catch2**：更丰富的断言（`EXPECT_EQ` 失败时打印两边的值）、自动注册用例、失败报告更友好。但骨架和上面完全一样：断言 + 退出码。先用零依赖版建立心智，框架随用随学。

## 17. Sanitizer 与 clang-tidy：自动化的猎手

### 17.1 Sanitizer：M4 的内存错误，在这里有了自动猎手

M4 讲了 C/C++ 全部经典内存错误：越界、use-after-free、double free、泄漏。当时的手段是「用 RAII 从源头避免」；但人还是会犯错，剩下的漏网之鱼谁来抓？**Sanitizer**——编译器在生成的代码里**插桩**（每次内存访问前后插入检查代码），运行时逐次核验，一越界当场报告「哪一行、访问了哪块内存、这块内存在哪里分配/在哪里释放」。这比 `printf` 大海捞针是代差级的提升，M4 的知识在这里完成闭环：**RAII 防患于未然，Sanitizer 抓漏网之鱼**。

三大件：

- **ASan（AddressSanitizer）**：堆/栈越界、use-after-free、double free、泄漏。
- **UBSan（UndefinedBehaviorSanitizer）**：有符号溢出、空指针解引用、非法位移等未定义行为。
- **TSan（ThreadSanitizer）**：**数据竞争**——第 12 节的竞争 bug 靠肉眼几乎不可复现，TSan 直接指出「这两行在无同步地访问同一地址」。（TSan 与 ASan 不能同开；MSVC 没有 TSan，需要用 clang/g++ 跑。）

开关：

```bash
# g++ / clang（开发机或 CI 上）
g++ -std=c++17 -g -fsanitize=address,undefined main.cpp -o app
clang++ -std=c++17 -g -fsanitize=thread main.cpp -o app     # 查数据竞争

# MSVC（VS 2019 16.9+，支持 ASan）
cl /EHsc /std:c++17 /Zi /fsanitize=address main.cpp
```

习惯：**测试永远开着 ASan 跑**（debug/CI 开，release 关——插桩有 2 倍左右开销）。测试覆盖到的每一行都被自动核验过内存安全，这就把「没有内存错误」从祈祷变成了流程保证。

### 17.2 clang-tidy：不运行程序的审查员

Sanitizer 要跑起来才抓 bug；`clang-tidy` **只读代码**就给意见：可疑写法（`=` 写成了条件、悬垂引用模式）、性能隐患（循环里按值拷大对象）、现代化建议（裸 `new` → `make_unique`、`0` → `nullptr`、裸循环 → range-for），很多还能 `--fix` 自动改。

```bash
clang-tidy main.cpp -- -std=c++17
```

分工：编译器警告管「语言规则边缘」（所以我们一直开 `/W4`），clang-tidy 管「最佳实践」，Sanitizer 管「运行期实锤」。三层全上、挂进 CI，代码质量就不再依赖「review 的人当天眼神好不好」——还是那条主线。

---

## 18. 常见坑

1. **对空 `optional` 用 `*opt`** → UB（那块空间没构造过 T）。先 `if (opt)`，或用 `value_or`；`value()` 至少抛异常不静默。
2. **`variant` 用 `std::get<T>` 取错类型** → 抛 `bad_variant_access`。不确定就 `get_if` / `holds_alternative`；要穷尽处理用 `visit`。
3. **`string_view` 存进成员/返回/指向临时** → 悬垂 UB。它不拥有数据——只传参，不存储。
4. **把 `sv.data()` 当 C 字符串** → 它不保证 `\0` 结尾。交给 C API 前先转 `std::string`。
5. **结构化绑定遍历大对象用 `auto [k, v]`** → 整体拷贝一份。遍历首选 `const auto& [k, v]`。
6. **`catch` 按值捕获** → 派生类异常被切片（M3）+ 多余拷贝。永远 `catch (const std::exception&)`。
7. **异常逃出析构函数** → 栈展开中再抛 = 双异常 = `terminate`。析构里吞掉或记录，绝不外抛。
8. **拿异常当正常控制流**（「没查到」也 throw）→ 又慢又乱。预期内的失败用 `optional`/错误码。
9. **移动构造忘标 `noexcept`** → vector 扩容默默退回拷贝，性能白丢（M5 的伏笔，本模块给了原理）。
10. **`std::thread` 析构前没 join/detach** → 直接 `terminate`。规则：能 join 就 join。
11. **手动 lock/unlock，中途 return 或抛异常** → 锁永不归还，全体睡死。永远 `lock_guard`/`unique_lock`。
12. **多把锁各处加锁顺序不一致** → 死锁（A 持 1 等 2，B 持 2 等 1）。固定全局加锁顺序，或 `std::scoped_lock` 一次锁齐。
13. **用两次原子操作拼「检查再行动」** → 每步原子、组合不原子。复合不变量必须 mutex 圈整段。
14. **`cv.wait` 不带谓词** → 虚假唤醒 + 唤醒后条件已被别人改变，两头挨打。永远谓词循环等待。
15. **生产者结束后消费者睡死** → 谓词里没有退出条件。谓词写 `!q.empty() || done`，收工时 `notify_all`。

## 19. 高频面试点（附答案要点）

- **`optional` 解决什么问题？和返回 -1 比好在哪？** 把「可能没值」写进类型。哨兵值占用合法值域且忘检查无人知；optional 签名自说明、空不占 T 值域。内部是 T 的空间 + bool，值语义不堆分配。空时 `*opt` 是 UB，`value()` 抛异常。
- **`variant` 和 C union 的区别？`visit` 好在哪？** variant = 编译器自动维护 tag 的 union（布局：最大成员 + 类型下标）。union 读错成员是 UB、tag 靠手动同步；variant 读错抛异常、tag 不归你管。`visit` 提供穷尽检查：新增备选类型后，visitor 少个分支直接编译失败。
- **`string_view` 是什么？为什么不能持有？** 指针 + 长度的非拥有视图，即 C 的 `(char*, len)` 惯用法类型化。不拥有 → 数据先死则悬垂 → 只用于传参，不存储、不返回、不指向临时。另注意不保证 `\0` 结尾。
- **`constexpr` 函数一定在编译期执行吗？** 不。它是「两栖」的：实参为编译期常量且结果用于常量上下文才编译期算，否则退化为普通运行期函数。vs 宏：有类型、有作用域、无重复求值。
- **`if constexpr` 和普通 `if` 的本质区别？** 普通 if 两个分支都要编译；`if constexpr` 假分支不实例化，允许分支里写只对特定类型合法的代码。模板按类型分派的标准做法。
- **`enum class` 三个好处？** 不隐式转整数（要 `static_cast`）、枚举名有作用域不撞名、可指定底层类型。底层仍是整数。
- **throw 之后发生什么？** 三步：异常对象拷到运行时管理的存储（要活过栈展开）→ 栈展开，逐帧按逆序析构已构造的局部对象 → 匹配第一个类型兼容的 catch；无人接住则 `terminate`。
- **RAII 和异常安全什么关系？** 栈展开保证局部对象析构 = 异常是 RAII 覆盖的第三条离开路径（正常返回、提前 return 之外）。资源全在 RAII 对象里，异常路径就不写清理代码也不漏。裸资源 + 异常 = 泄漏放大器。
- **异常安全三级别？** 基本保证：不漏资源、对象合法但可能已变；强保证：要么成功要么回到原状（copy-and-swap 实现，代表 `push_back`）；不抛保证：`noexcept`（析构、swap、移动应达到）。
- **`noexcept` 有什么用？违约会怎样？** 对外承诺不抛，违约直接 `terminate` 不展开。价值：调用方可依赖它决策——vector 扩容仅当元素移动为 noexcept 才用移动（移动搬一半抛异常无法回滚，拷贝可以），否则退回拷贝。
- **异常 vs 错误码怎么选？** 预期内失败（查不到、输入非法）→ optional/错误码；罕见且就地处理不了的（构造失败、资源耗尽）→ 异常。异常正常路径近零开销、抛出时开销大；热路径和跨 C 边界用错误码。
- **`thread` 不 join 会怎样？join 和 detach 区别？** 析构时既没 join 也没 detach → `terminate`。join 阻塞等结束；detach 放生后台（有悬垂风险，慎用）。传参默认拷贝，传引用要 `std::ref`。
- **什么是数据竞争？为什么是 UB？** ≥2 线程无同步访问同一内存且至少一写。`++counter` 是读-改-写三步，交错导致更新覆盖丢失；更深层：编译器按单线程假设优化（寄存器缓存、重排、折叠循环），有竞争时行为与源码脱钩，故标准定为 UB。
- **什么是 RAII 锁？`lock_guard` vs `unique_lock`？** 构造加锁、析构解锁，return/异常路径都保证释放——RAII 思想应用于锁。`lock_guard` 轻量不可中途解锁，日常首选；`unique_lock` 可中途解锁/延迟上锁/转移，配 `condition_variable` 必须用它。
- **`atomic` 和 `mutex` 怎么选？** atomic = 单条不可分割硬件指令（如 x86 `lock xadd`），开销小不睡眠，适合单变量计数/标志；mutex 保护任意长临界区和多变量一致性。两次原子操作拼不出原子的「检查再行动」——复合不变量必须 mutex。
- **`condition_variable` 为什么必须带谓词的 wait？** 一防虚假唤醒（OS 允许无通知自醒）；二防「唤醒到抢回锁之间条件又被改」（两个消费者被同时唤醒，后拿到锁的发现货已被取走）。带谓词 wait = 醒来持锁重查条件的 while 循环。
- **为什么 wait 要用 `unique_lock`？** wait 内部要「原子地解锁+睡眠」、醒后重新加锁，需要能中途解锁/加锁的锁包装，`lock_guard` 做不到。
- **CMake 是构建系统吗？** 不是，是构建系统**生成器**：读声明式的 CMakeLists（目标 + 依赖关系），生成本地构建系统（VS 工程/Makefile/Ninja）。两步：`cmake -S . -B build` configure，`cmake --build build` 编译。
- **ASan/TSan 是什么原理？** 编译期插桩 + 运行时核验。ASan 抓越界/use-after-free/泄漏，TSan 抓数据竞争，UBSan 抓未定义行为。测试常开 ASan，release 关（约 2 倍开销）。

## 20. 编译提醒

单文件练习（x64 Native Tools 命令行）：
```
cl /EHsc /std:c++17 /W4 文件名.cpp
```
多文件：
```
cl /EHsc /std:c++17 /W4 main.cpp counter.cpp
```
CMake 项目（mini 项目用这个）：
```
cmake -S . -B build
cmake --build build
ctest --test-dir build
```
开 ASan 排错（MSVC）：
```
cl /EHsc /std:c++17 /Zi /fsanitize=address 文件名.cpp
```
> 注：本机 bash 沙箱用 g++（ucrt64）验证语法：`g++ -std=c++17 -Wall -Wextra -fsyntax-only 文件.cpp`（线程相关可加 `-pthread`）。沙箱不能链接生成 exe，只做语法/编译检查；完整构建运行在本机 MSVC / CMake 下做。

---

## 21. 收官：一条主线走完 M1~M8

回头看，整个系列其实一直在讲同一件事：

- **M2**：对象的生命周期有明确的起点和终点——构造函数和析构函数**必然**被调用；
- **M4**：既然析构必然执行，就把资源释放放进去——RAII，「忘了 free」这条路径从此不存在；
- **M5**：资源可以转移而不必复制——移动语义，而 `noexcept` 的移动才能被容器放心使用；
- **M8**：同一个机制覆盖到错误路径（栈展开时 RAII 照常兑现 → 异常安全）和并发（`lock_guard` → 忘不了解锁），再由工具链补上最后一段（Sanitizer 抓漏网、CTest 自动回归）。

从头到尾就是一个思想：**把正确性从「程序员记得做」变成「编译器和工具保证做」。** 你现在掌握的不是一堆零散语法，是这套思想在类型、资源、错误、并发、工程五个领域的完整落地——这也是你读任何现代 C++ 代码库时能看懂「它为什么这么设计」的钥匙。

接下来是 **Capstone：WAV 文件解析器**，M1~M8 的知识在那里全部上场：`std::vector<uint8_t>` 装字节流（M7）、RAII 文件句柄（M4）、`std::optional` 表达「解析失败」（本模块）、`enum class : uint16_t` 对应 WAV 头里的格式标签字段（本模块）、结构化绑定拆解析结果、零依赖单测验证解析逻辑 + CMake 组织工程（本模块）。到那时你会发现：Capstone 里几乎没有新知识，只有旧知识的组合——这正是「学完了」的标志。

先打开 `exercises.md`，把十道练习和 mini 项目（线程安全的生产者-消费者队列——第 14 节的骨架就是它的蓝图）做完，再进 Capstone。
