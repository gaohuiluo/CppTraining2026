# M5 拷贝、移动与 Rule of 0/3/5

> 在 C 里，`struct b = a;` 就是一次 memcpy，编译器不多问一句。C++ 把「复制一个对象」变成了一次**函数调用**——这个函数由你定义、由编译器在恰当的时机自动调用。调用谁、怎么写、什么时候能干脆不调用（移动、RVO），就是本模块的全部内容。这条线从「double free 为什么必然发生」一路推到「为什么现代 C++ 首选一个特殊成员函数都不写」，中间每一步都是上一步的必然结果——不需要背，只需要跟着推。

---

## 0. 一句话总览

**拷贝是「给对象配一个独立副本」，移动是「把将死对象的家当抢过来」；类持有资源时你必须接管拷贝（否则 double free），而移动、RVO、Rule of 0 都是在保证正确的前提下，把拷贝的代价一步步砍到零。**

本模块的主线是一条因果链，每一节解决上一节留下的问题：

```text
 C 的 = 是 memcpy ───► C++ 默认拷贝 = 逐成员拷贝                [第 1 节]
        │
        ▼  成员是裸指针？逐成员拷贝只拷地址！
 两个对象指向同一块堆内存 ──► 各析构一次 ──► double free        [第 2 节]
        │
        ▼  解法：自己写深拷贝
 Rule of 3：析构 + 拷贝构造 + 拷贝赋值，总是三个一起出现        [第 3 节]
        │
        ▼  拷贝赋值比拷贝构造难：旧资源 / 自赋值 / 异常安全
 copy-and-swap：用一个副本 + 一次交换解决全部三个难题           [第 4 节]
        │
        ▼  新问题：给「马上要死的临时对象」做深拷贝是纯浪费
 左值 / 右值：语言层面区分「还活着的」和「将死的」              [第 5 节]
        │
        ▼  对将死的：偷指针 + 源置空
 移动构造 / 移动赋值                                            [第 6 节]
        │
        ▼  想对左值也用移动？给它换个「类型标签」就行
 std::move 与重载决议                                           [第 7 节]
        │
        ▼  比移动更快的是根本不发生
 RVO / NRVO / C++17 强制拷贝消除                                [第 8 节]
        │
        ▼  vector 扩容时敢不敢用你的移动？
 noexcept 移动                                                  [第 9 节]
        │
        ▼  收束：这五个函数到底该不该写
 Rule of 5 ──► Rule of 0 ──► =default / =delete ──► unique_ptr [第 10 节]
```

记住这张图。后面任何一节读迷路了，回来看看自己在链条的哪个位置。

---

## 1. 起点：C 的 `=` 是 memcpy，C++ 的默认拷贝是「逐成员拷贝」

一切从「默认行为到底是什么」说起——不搞清楚编译器背着你做了什么，就没法判断它做得对不对。

C 里结构体赋值，语义就是逐字节复制，等价于一次 `memcpy`：

```c
typedef struct { int* data; int size; } Buffer;

Buffer b = a;   /* 逐字节拷贝：b.data 和 a.data 是同一个地址 */
```

C++ 不一样。你什么都不写时，编译器会为类自动生成**拷贝构造函数**和**拷贝赋值运算符**，它们的行为是**逐成员拷贝（memberwise copy）**：对每个成员，调用**它自己的**拷贝方式。

- `int`、`double` 等标量成员：复制值（和 C 一样）。
- `std::string`、`std::vector` 等类类型成员：调用**它们的**拷贝构造/拷贝赋值（递归下去，string 会正确复制自己的堆内存）。
- **裸指针成员：指针也是标量，复制的是地址本身**——两份指针，一块内存。

| | C 的 `=` | C++ 默认拷贝 |
|---|---|---|
| 本质 | 逐字节 memcpy | 逐成员，递归调用成员的拷贝函数 |
| `std::string` 成员 | （C 没有）| 正确深拷贝自己的内容 |
| 裸指针成员 | 拷地址 | **一样是拷地址** |
| 能否自定义 | 不能 | 能——拷贝就是个函数，你可以重写它 |

所以：**成员全是「懂得拷贝自己」的类型时，C++ 默认拷贝是完全正确的；一旦出现裸指针，它就退化成和 C 的 memcpy 一样危险。** 这句话是全模块的种子，第 2 节让它发芽。

### 1.1 先分清：初始化是「构造」，`=` 给活着的对象才是「赋值」

从 C 过来最容易懵的一点——`=` 这个符号在 C++ 里对应两个**不同的函数**：

```cpp
Buffer a(100);
Buffer b = a;    // b 正在诞生 → 拷贝构造（这里的 = 是初始化语法，不是赋值！）
Buffer c(50);
c = a;           // c 早就活着了 → 拷贝赋值 operator=
```

判据只有一条：**等号左边的对象是不是正在诞生**。正在诞生 → 构造；已经活着 → 赋值。这个区分贯穿全篇（第 4 节你会看到，赋值恰恰因为「左边已经活着」而比构造难）。

### 1.2 拷贝构造在哪些时机被调用

```cpp
void byValue(Buffer b);          // 按值传参
Buffer a(100);

Buffer b = a;       // (1) 拷贝初始化
Buffer c(a);        // (2) 直接初始化，同样是拷贝构造
byValue(a);         // (3) 传参：形参是实参的副本
Buffer d = make();  // (4) 按值返回：理论上要拷贝，实际通常被消除（第 8 节）
```

vs C：C 里传参、返回结构体也是 memcpy，你无从干预；C++ 把这四个时机都变成了「调用拷贝构造」，于是你有了插手的机会。

### 1.3 为什么拷贝构造的参数必须是 `const Buffer&`

- 必须是**引用**：如果写 `Buffer(Buffer other)` 按值传参，那么「传参」本身又需要拷贝一次 → 又调用拷贝构造 → 无限递归。编译器直接拒绝这种签名。
- 加 **const**：一是拷贝不应该改动源对象；二是 `const&` 能绑定临时对象（为什么能，第 5 节从绑定规则讲清），`Buffer b = Buffer(100);` 这种写法才合法。

---

## 2. 推演一次崩溃：逐成员拷贝 × 持有资源 = double free

第 1 节留下的引线：裸指针成员的默认拷贝只拷地址。现在把它接到一个真实的类上，一步一步推出崩溃——目标是让你看到，这次崩溃**不是运气差，是三个事实叠加后的必然**。

```cpp
// 反例：能编译，运行必崩。先别急着跑，跟着下面的内存图走一遍。
class Buffer {
public:
    explicit Buffer(int n) : size_(n), data_(new int[n]) {}
    ~Buffer() { delete[] data_; }
    // 没写拷贝构造/拷贝赋值 → 编译器生成逐成员拷贝的版本
private:
    int  size_;
    int* data_;
};

int main() {
    Buffer a(4);
    Buffer b = a;   // 默认拷贝构造：逐成员拷贝
}                   // 作用域结束，灾难在这里
```

**第一步**：`Buffer a(4);` 构造完成后的内存布局——

```text
栈                          堆
a.size_ = 4
a.data_ ──────────────────► [ 0 ][ 0 ][ 0 ][ 0 ]   (16 字节，设地址为 0x5000)
```

**第二步**：`Buffer b = a;` 触发编译器生成的拷贝构造，行为是 `b.size_ = a.size_; b.data_ = a.data_;`——注意最后一句拷的是**指针的值**，也就是 `0x5000` 这个地址：

```text
栈                          堆
a.size_ = 4
a.data_ ──────────┐
                  ├───────► [ 0 ][ 0 ][ 0 ][ 0 ]   (还是 0x5000，同一块！)
b.data_ ──────────┘
b.size_ = 4
```

**第三步**：作用域结束。局部对象按构造的**逆序**析构（后构造的先析构），先 `b` 后 `a`：

```text
~b 执行 delete[] b.data_;   → 0x5000 归还给堆分配器，这块内存不再属于你

a.data_ ──────────────────► ？？？   (0x5000 已释放，a.data_ 成了悬垂指针)

~a 执行 delete[] a.data_;   → 对同一地址第二次 delete → double free，未定义行为
```

把推理压缩成三个事实，缺一个都不崩，凑齐了必崩：

1. 默认拷贝对指针成员只拷地址 → 两个对象共享同一块堆内存；
2. 两个对象**各自**都会析构，各执行一次 `delete[]`；
3. 同一块内存 `delete` 两次是未定义行为（典型表现：堆分配器的簿记结构被破坏，程序崩溃）。

而且 double free 还只是最响亮的死法。就算侥幸没崩：改 `b` 的数据会「莫名其妙」影响 `a`（两者是别名）；`b` 先析构后再用 `a` 的数据就是 use-after-free。**共享 + 独占式释放，怎么走都是死路。**

vs C：这就是 C 里「struct 带指针不能直接 `=`」的老坑。区别在于 C 里你至少看得见 `=`，而 C++ 里传参、返回、放进容器都在暗中拷贝——坑更隐蔽了。但 C++ 也给了 C 没有的解药：拷贝是函数，函数可以重写。

---

## 3. 解法：深拷贝——以及 Rule of 3 为什么是「三个一起」

第 2 节的死因是「两个对象共享一块内存」。解法自然就是：**拷贝时给副本分配自己的内存，把内容复制过去**——深拷贝。

```cpp
#include <iostream>
#include <algorithm>   // std::copy
#include <cstddef>

class Buffer {
public:
    explicit Buffer(std::size_t n) : size_(n), data_(new int[n]()) {
        std::cout << "构造     data_=" << data_ << "\n";
    }
    ~Buffer() {
        std::cout << "析构     data_=" << data_ << "\n";
        delete[] data_;
    }
    // 深拷贝构造：先给自己开一块新内存，再逐元素复制内容
    Buffer(const Buffer& other)
        : size_(other.size_), data_(new int[other.size_]) {
        std::copy(other.data_, other.data_ + size_, data_);
        std::cout << "拷贝构造 data_=" << data_ << "（新地址）\n";
    }
    Buffer& operator=(const Buffer& other);   // 拷贝赋值另有讲究，第 4 节写它

    void set(std::size_t i, int v) { data_[i] = v; }
    int  get(std::size_t i) const  { return data_[i]; }
private:
    std::size_t size_;
    int*        data_;
};

int main() {
    Buffer a(4);
    a.set(0, 1);
    Buffer b = a;     // 深拷贝：b 拿到自己的内存
    b.set(0, 999);
    std::cout << a.get(0) << " " << b.get(0) << "\n";   // 1 999：互不影响
}
```

跑一下你会看到两次构造打印的 `data_` 地址**不同**——这正是「各持一份」的物证。带打印的 Buffer 是本模块的看家手法，后面每一节都靠它把「编译器暗中调了什么」变成肉眼可见的输出。

| | 浅拷贝（默认生成） | 深拷贝（手写） |
|---|---|---|
| 指针成员 | 拷地址，共享内存 | 新分配一块，复制内容 |
| 析构 | double free / 悬垂 | 各管各的，安全 |
| 改一个 | 影响另一个 | 互不影响 |
| 开销 | O(1) | O(n)：分配 + 复制 |

### 3.1 Rule of 3：三个函数是同一个事实的三个投影

现在可以回答一个经典问题了：为什么「析构、拷贝构造、拷贝赋值」总是三个一起出现（C++98 时代的 Rule of 3）？

因为它们**都源于同一个事实：这个类拥有资源**。

- 类拥有资源（`new` 出来的内存、文件句柄、锁……）→ 释放责任在自己 → **必须手写析构**；
- 类拥有资源 → 默认的逐成员拷贝会导致共享 + 双重释放（第 2 节的推演）→ **必须手写拷贝构造**；
- 同理 `c = a;` 也会走默认浅拷贝 → **必须手写拷贝赋值**。

反过来推也成立：如果你发现自己在写析构函数，就该条件反射地问「那默认拷贝还对吗？」——答案几乎总是「不对」。**三个函数不是三条规矩，是「类拥有资源」这一个事实投在三个方向上的影子。** 少写任何一个，影子就缺一块，资源管理就有漏洞。

上面的示例故意只写了拷贝构造、把拷贝赋值留成声明——因为赋值比构造多出三件麻烦事，值得单开一节。

---

## 4. 拷贝赋值：多出的三件事，以及 copy-and-swap 为什么优雅

第 3 节欠的账：`operator=`。它和拷贝构造的差别只有一个，但这个差别引出三件额外的事——**赋值时左边的对象已经活着，手里攥着一块旧资源**。

### 4.1 三件多出来的事

1. **释放旧资源**：`c = a;` 之后 `c` 原来那块内存没人指了，不 `delete[]` 就是泄漏。
2. **防自赋值**：`a = a;`（更常见的是通过指针/引用绕一圈的间接自赋值）。如果流程是「先释放自己的，再从 other 拷贝」，自赋值时 other 就是自己——你刚把数据源释放了，再从那里拷贝就是读已释放内存。
3. **异常安全**：`new` 可能抛出 `bad_alloc`（M4 的对比表提过）。如果顺序是「先 delete 旧的，再 new 新的」，`new` 一抛，`data_` 已经指向释放掉的内存、新内存又没拿到——对象残废，连析构都会二次释放。

### 4.2 四步走的手写版本

三件事逐一化解：先检查自赋值；**先分配复制、后释放旧的**（new 失败时 `*this` 分毫未动，这就是「强异常保证」——操作要么完整成功，要么对象保持原样）：

```cpp
Buffer& Buffer::operator=(const Buffer& other) {
    if (this == &other) return *this;         // 1. 自赋值检查
    int* p = new int[other.size_];            // 2. 先分配 + 复制（失败则 *this 完好）
    std::copy(other.data_, other.data_ + other.size_, p);
    delete[] data_;                           // 3. 再释放旧资源
    data_ = p;                                // 4. 接管，更新 size_
    size_ = other.size_;
    return *this;                             //    返回 *this 支持链式 a = b = c（M2）
}
```

能写对，但你得时刻记着三根弦：检查了没？顺序对没对？漏了 size_ 没？——容易错的代码，就值得找一个「结构上就不会错」的写法。

### 4.3 从三件事推导出 copy-and-swap

回头看三件事的本质：

- 异常安全的要点是「**在动 `*this` 之前，把可能失败的活（分配+复制）干完**」；
- 自赋值的危险源于「**释放的和拷贝的源是同一块**」——如果拷贝的源是个独立副本，就无所谓；
- 旧资源要有人**顺手带走**。

三条合起来指向同一个形状：**先造一个完整的副本（可能失败的活全在这一步，`*this` 未动）；然后把 `*this` 和副本交换（只换指针，不可能失败）；最后让副本带着旧资源自然析构。** 这就是 copy-and-swap。而「造副本」这一步都不用自己写——把参数改成**按值传递**，编译器就会在调用处用拷贝构造替你造好：

```cpp
#include <iostream>
#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

class Buffer {
public:
    Buffer(std::size_t n, std::string tag)
        : size_(n), data_(new int[n]()), tag_(tag) {
        std::cout << "[构造] " << tag_ << "\n";
    }
    ~Buffer() {
        std::cout << "[析构] " << tag_ << "\n";
        delete[] data_;
    }
    Buffer(const Buffer& other)
        : size_(other.size_), data_(new int[other.size_]),
          tag_("copy_of_" + other.tag_) {
        std::copy(other.data_, other.data_ + size_, data_);
        std::cout << "[拷贝构造] " << tag_ << "\n";
    }

    // 交换只动指针和内部簿记，O(1) 且不会抛异常
    friend void swap(Buffer& x, Buffer& y) noexcept {
        using std::swap;
        swap(x.size_, y.size_);
        swap(x.data_, y.data_);
        swap(x.tag_,  y.tag_);
    }

    // copy-and-swap：注意参数是按值传递！拷贝发生在调用处
    Buffer& operator=(Buffer other) noexcept {
        std::cout << "[operator=] 与 " << other.tag_ << " 交换\n";
        swap(*this, other);
        return *this;
    }   // other 离开作用域，带着 *this 的旧资源析构

private:
    std::size_t size_;
    int*        data_;
    std::string tag_;
};

int main() {
    Buffer a(10, "a");
    Buffer b(20, "b");
    b = a;    // 观察输出顺序：拷贝构造(传参) → 交换 → 析构(副本带走 b 的旧内存)
    a = a;    // 自赋值也安全：拷出副本再和副本交换，逻辑天然正确
}
```

`b = a;` 那一行的输出会是这样一条流水线：

```text
[拷贝构造] copy_of_a      ← 1. 按值传参：调用处拷贝构造出副本 other
[operator=] 与 copy_of_a 交换 ← 2. swap：b 拿到新内存，other 拿到 b 的旧内存
[析构] b                  ← 3. other 析构，旧内存被顺手带走
```

三件事的账逐条对上：

| 难题 | 四步走版本 | copy-and-swap |
|---|---|---|
| 异常安全 | 靠「先分配后释放」的顺序纪律 | 拷贝失败发生在进函数体之前，`*this` 天然未动 |
| 自赋值 | 靠 `if (this == &other)` 记得写 | 交换的是副本，结构上就正确 |
| 释放旧资源 | 靠 `delete[]` 记得写 | 副本析构自动带走 |
| 代码行数 | 7~8 行，三处易错 | 3 行，没有可漏的步骤 |

唯一的代价：自赋值时 copy-and-swap 也会老老实实做一次深拷贝（四步走版本可以提前 return）。自赋值罕见到可以忽略，这笔交易几乎总是划算的。另外按值传参还埋着一个彩蛋：等第 6 节引入移动构造后，`b = std::move(x)` 时参数 `other` 会用**移动**构造——同一个 `operator=` 免费兼任移动赋值。

到这里，「正确的拷贝」已经完整了。但接下来要对「拷贝」本身发起一次灵魂拷问。

---

## 5. 转折：有些拷贝是纯浪费——左值与右值

拷贝写对了，新问题浮出水面：**有些拷贝从一开始就不该发生**。

```cpp
Buffer make() { return Buffer(1000000, "big"); }

Buffer b(1, "b");
b = make();   // make() 返回一个临时 Buffer
```

按第 4 节的机制，`b = make()` 要对这个临时对象做深拷贝：分配 4 MB、复制一百万个 int——然后临时对象在这条语句结束时**立刻析构**，它那份原件白白烧掉。给一个下一毫秒就要火化的对象做全套克隆，图什么？直接把它的 `data_` 指针接过来不就完了？

想「接管」而不是「复制」，语言得先能回答一个问题：**我怎么知道赋值号右边的对象是「马上要死的临时」还是「还要继续活的变量」？** 掏空前者是白捡，掏空后者是抢劫。这个区分不能靠猜，得是类型系统里可判定的东西——这就是**值类别（value category）**存在的意义。

### 5.1 左值与右值：可操作的判据

标准里值类别细分五种，日常够用的是两大类：

- **左值（lvalue）**：有名字、占据一个可识别的内存位置、表达式结束后还活着。变量、`*p`、数组元素、返回引用的函数调用。
- **右值（rvalue）**：没名字的临时产物，所在的完整表达式一结束就销毁。字面量 `42`、`x + 5` 的结果、`Buffer(100)` 这种临时对象、按值返回的函数调用结果。

实用判据两条，命中率极高：

1. **有名字的基本是左值**（哪怕它绑定的是临时物——名字意味着后面还可能被用到）；
2. **能合法取地址 `&` 的是左值**：`&x` 合法，`&(x + 5)`、`&42` 编译不过。

```cpp
int x = 10;
int y = x + 5;    // x、y 是左值；x + 5 的结果是右值
Buffer make();
Buffer b = make();  // make() 的返回值是右值（将死的临时）；b 是左值
```

注意值类别说的是**表达式**，不是变量本身：`x` 这个表达式是左值，`x + 0` 就是右值。

### 5.2 三种引用的绑定规则

有了「将死/还活着」的区分，还需要**能分别抓住它们的把手**——引用。C++ 有三种：

| 实参是 ↓ 形参是 → | `T&` | `const T&` | `T&&` |
|---|---|---|---|
| 左值 | ✓ | ✓ | ✗ |
| const 左值 | ✗ | ✓ | ✗ |
| 右值（临时） | ✗ | ✓（只读） | ✓ |

- `T&`（左值引用，M1 的老朋友）：只绑左值。临时对象绑不上——语言不让你拿可写引用去指一个马上消失的东西。
- `const T&`：**万能接受者**，左值右值通吃。这是 C++98 的历史决定：为了让 `f(const string&)` 能接收 `f("hello")` 产生的临时对象而不用拷贝，规定 const 左值引用可以绑定右值（还会把临时对象的生命期延长到引用的作用域）。拷贝构造用它当参数，就是靠这条规则才能拷贝临时对象。但它是**只读**的——你没法通过它掏空临时对象。
- `T&&`（**右值引用**，C++11 新引入）：**只绑右值**，而且**可写**。这正是缺的那块拼图：一个「专门抓住将死对象、还允许你动它家当」的把手。

`T&&` 真正的威力在于**重载**。同一个函数名写两个版本，编译器按实参的值类别自动分流：

```cpp
#include <iostream>
#include <utility>

void probe(const int& x) { std::cout << x << " -> 左值版本\n"; }
void probe(int&& x)      { std::cout << x << " -> 右值版本\n"; }

int main() {
    int a = 5;
    probe(a);            // 左值 → const int& 版本
    probe(10);           // 右值 → int&& 版本
    probe(a + 1);        // 表达式结果是临时 → 右值版本
    probe(std::move(a)); // 被强转成右值 → 右值版本（std::move 是什么，第 7 节）
}
```

注意 `probe(10)`：`const int&` 和 `int&&` 都能绑定右值，但**重载决议规定右值实参优先匹配 `T&&` 版本**——`T&&` 是更精确的匹配，`const T&` 是兜底。这条优先级就是下一节移动构造能「抢在」拷贝构造前面被选中的机制。

vs C：C 只有值和指针，「这个表达式的结果还能活多久」在类型系统里完全没有体现，所以 C 永远只能保守地复制。值类别是 C++ 为「安全地压榨临时对象」专门修的语法地基。

---

## 6. 移动：偷指针 + 源置空

地基修好了：`T&&` 能精确捕获将死的对象，还允许改动它。现在实现第 5 节的野心——不复制，直接**接管**。

### 6.1 移动构造与移动赋值

```cpp
// 移动构造：接管 other 的资源，O(1)，不分配不复制
Buffer(Buffer&& other) noexcept
    : size_(other.size_), data_(other.data_) {   // 偷：直接拿走指针
    other.data_ = nullptr;                       // 置空：关键一步，见 6.2
    other.size_ = 0;
}

// 移动赋值：先放下自己的旧资源，再接管 other 的
Buffer& operator=(Buffer&& other) noexcept {
    if (this == &other) return *this;   // 防自移动（x = std::move(x) 罕见但防住不亏）
    delete[] data_;                     // 释放自己的旧资源（否则泄漏，同第 4 节）
    data_ = other.data_;                // 偷
    size_ = other.size_;
    other.data_ = nullptr;              // 置空
    other.size_ = 0;
    return *this;
}
```

两个签名细节：参数是 `Buffer&&` 且**没有 const**——移动就是要改动源对象，const 会把这条路堵死（第 7 节有个坑正源于此）；`noexcept` 先照写，第 9 节解释它为什么远不止是好习惯。

### 6.2 为什么必须把源对象置空——用第 2 节的逻辑再推一遍

「偷指针」之后的现场：`this->data_` 和 `other.data_` 指向**同一块内存**。眼熟吗？这正是第 2 节浅拷贝崩溃现场的第二步！`other` 虽然将死，但**将死也是要走析构函数的**——如果不置空，`other` 析构时 `delete[]` 那块刚被接管的内存，新对象手里立刻变成悬垂指针，自己析构时再 delete 一次，double free 全套剧本重演。

置空一步把剧本改写：`other.data_ = nullptr` 之后，`other` 析构执行的是 `delete[] nullptr`——C++ 标准保证 delete 空指针是无害的空操作。于是「共享」变成了「交接」：**同一时刻，这块内存永远只有一个主人**。这也是为什么说移动 = 偷指针 + 源置空，两步缺一不可，后一步是前一步的安全锁。

### 6.3 移动后的源对象：有效但未指定

移动完的 `other` 是什么状态？标准的说法叫**有效但未指定（valid but unspecified）**：

- **有效**：它仍是个结构完好的对象——可以安全析构（上面推过了），可以被重新赋值（`a = Buffer(50);` 给它新生命），可以调用不依赖具体值的成员函数。
- **未指定**：不要再依赖它原来的值。我们自己的 Buffer 移动后是明确的空（我们亲手置的空），但对标准库类型，「移动后为空」只是常见实现，**不是标准承诺**——`std::string` 被移动后多半是空串，但写出依赖这一点的代码就是在赌实现细节。

```cpp
Buffer a(100);
Buffer b = std::move(a);   // a 的资源被搬进 b
// 此后合法：a = Buffer(50);   （重新赋值）
//          让 a 自然析构
// 不合法（逻辑上）：假设 a 还持有那 100 个元素
```

### 6.4 亲眼看一次：拷贝 vs 移动

把第 3~4 节的拷贝二人组和本节的移动二人组装进同一个 Buffer，每个函数带打印——这就是 Rule of 5 的完整形态（mini 项目要你亲手写的就是它）。先预测输出再运行：

```cpp
#include <iostream>
#include <algorithm>
#include <cstddef>
#include <utility>

class Buffer {
public:
    explicit Buffer(std::size_t n) : size_(n), data_(new int[n]()) {
        std::cout << "构造      data_=" << data_ << "\n";
    }
    ~Buffer() {
        std::cout << "析构      data_=" << data_ << "\n";
        delete[] data_;                            // data_ 为 nullptr 时也安全
    }
    Buffer(const Buffer& other)                    // 拷贝构造：深拷贝（第 3 节）
        : size_(other.size_), data_(new int[other.size_]) {
        std::copy(other.data_, other.data_ + size_, data_);
        std::cout << "拷贝构造  新分配 data_=" << data_ << "\n";
    }
    Buffer& operator=(const Buffer& other) {       // 拷贝赋值：四步走（第 4 节）
        std::cout << "拷贝赋值\n";
        if (this == &other) return *this;
        int* p = new int[other.size_];
        std::copy(other.data_, other.data_ + other.size_, p);
        delete[] data_;
        data_ = p;
        size_ = other.size_;
        return *this;
    }
    Buffer(Buffer&& other) noexcept                // 移动构造：偷 + 置空
        : size_(other.size_), data_(other.data_) {
        other.data_ = nullptr;
        other.size_ = 0;
        std::cout << "移动构造  接管 data_=" << data_ << "\n";
    }
    Buffer& operator=(Buffer&& other) noexcept {   // 移动赋值
        std::cout << "移动赋值\n";
        if (this == &other) return *this;
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
        return *this;
    }
    bool empty() const { return data_ == nullptr; }
private:
    std::size_t size_;
    int*        data_;
};

int main() {
    Buffer a(4);
    Buffer b = a;              // 左值 → 拷贝构造：打印的地址是「新分配」的
    Buffer c = std::move(a);   // 右值 → 移动构造：打印的地址和 a 原来的相同！
    std::cout << "move 后 a.empty() = " << std::boolalpha << a.empty() << "\n";
    Buffer d(1);
    d = std::move(c);          // 移动赋值：d 的旧内存被析构打印之前就释放了
}
```

（`std::move` 是什么下一节才拆解，这里先当「把左值转成右值的开关」用。）两处最值得盯的输出：移动构造打印的地址**和 `a` 构造时一模一样**——同一块内存换了主人，这是「偷」的物证；`a.empty()` 为 true——这是「置空」的物证。

### 6.5 账本：移动到底省了什么

| 操作（100 万个 int，约 4 MB） | 拷贝 | 移动 |
|---|---|---|
| 堆分配 | 再要 4 MB | 无 |
| 数据复制 | 100 万次 | 无 |
| 实际动作 | O(n) | O(1)：抄两个成员 + 置空两个成员 |

回收第 4 节的彩蛋：copy-and-swap 的 `operator=(Buffer other)` 在类有了移动构造后，`b = std::move(x)` 时参数 `other` 直接**移动**构造——一个函数同时充当拷贝赋值和移动赋值，这是那种写法的又一重优雅。

---

## 7. std::move 的真相：一行运行期代码都没有

第 6 节的移动只在实参是右值时触发。但有时你手里是个**左值**，却明知道它后面不用了——比如要把局部变量转交给容器。能不能对左值也用移动？能，这就是 `std::move`——不过它的名字是 C++ 圈公认的起名事故。

### 7.1 它只是一个类型转换

**`std::move` 不移动任何东西。** 它约等于一个 `static_cast<T&&>`：

```cpp
std::move(a)   ≈   static_cast<Buffer&&>(a)
```

（它的真身是个一行的模板函数，涉及第 11 节的引用折叠，M6 会看全貌。）它编译后**零运行期代码**——不分配、不复制、不改动 `a` 的任何一个字节。它做的唯一一件事发生在**编译期**：把表达式的值类别从左值改成右值，从而**改变重载决议的选择**。

### 7.2 编译器怎么在拷贝和移动之间选

类同时有拷贝构造 `Buffer(const Buffer&)` 和移动构造 `Buffer(Buffer&&)` 时，规则就是第 5 节那张绑定表加一条优先级：

| 实参 | 能匹配谁 | 编译器选谁 |
|---|---|---|
| 左值 `a` | 只有 `const Buffer&` | 拷贝构造 |
| 右值（临时 / `std::move(a)`） | 两个都能绑 | **`Buffer&&` 更精确 → 移动构造** |

```cpp
Buffer a(100);
Buffer b = a;              // a 是左值 → 拷贝构造（深拷贝，O(n)）
Buffer c = std::move(a);   // 表达式变右值 → 移动构造（偷家，O(1)）
```

所以 `std::move` 的准确读法是一句**承诺**：「我保证之后不再用 `a` 的值了，重载决议请放心选移动版本。」真正搬家当的是你写的移动构造/移动赋值；`std::move` 只是把门票换成了右值票。推论：**move 完别再用源对象的值**（它已进入 6.3 的未指定状态）——这个责任在你，编译器只认票不验人。

### 7.3 坑：对 const 对象 move，静默退化为拷贝

用绑定规则自己推一遍：`const Buffer a` 经过 `std::move` 得到的是 `const Buffer&&`——const 右值引用。查表：

- 移动构造要 `Buffer&&`（非 const，它要改源对象）→ **const 的绑不上**；
- 拷贝构造的 `const Buffer&` 是万能接受者 → 绑上了。

```cpp
#include <iostream>
#include <string>
#include <utility>

int main() {
    const std::string s = "a string long enough to avoid SSO.....";
    std::string t = std::move(s);   // 反例：一个字都没警告，但实际是拷贝！
    std::cout << "s = \"" << s << "\"\n";   // s 完好如初——它根本没被移动
}
```

没有编译错误、没有警告，只是悄悄比你以为的慢——这类「静默退化」是最阴的坑。道理也正当：移动必须掏空源对象，const 对象承诺不可改，两者天然矛盾。教训：**打算移动的对象别声明成 const。**

---

## 8. RVO / NRVO：比移动更快的，是根本不发生

移动把 O(n) 砍成 O(1)。这节再砍一刀：在「函数按值返回」这个最常见的场景里，编译器连那个 O(1) 都想省掉——而你唯一要做的是**别帮倒忙**。

### 8.1 返回值优化是什么

`Buffer b = make();` 按第 1 节的说法要经历「函数里构造 → 拷贝/移动给返回值 → 拷贝/移动给 b」。但编译器有个釜底抽薪的办法：**让函数直接在 `b` 的内存位置上构造对象**，中间环节全部蒸发。这就是 RVO（Return Value Optimization）；针对具名局部变量的版本叫 NRVO（Named RVO）。

两种情形待遇不同：

- `return Buffer(100);`（返回无名临时）：**C++17 起这不再是优化，是语言规则**——保证零拷贝零移动（guaranteed copy elision），拷贝/移动构造压根不需要存在。
- `return local;`（返回具名局部变量）：NRVO，标准**允许但不强制**；MSVC、GCC、Clang 在开优化时基本都做。就算某个角落做不了，C++17 也规定这里把 `local` 当右值看待，兜底也是移动而非拷贝。

```cpp
#include <iostream>
#include <utility>

struct Noisy {
    Noisy()                 { std::cout << "构造\n"; }
    Noisy(const Noisy&)     { std::cout << "拷贝构造\n"; }
    Noisy(Noisy&&) noexcept { std::cout << "移动构造\n"; }
    ~Noisy()                { std::cout << "析构\n"; }
};

Noisy makePrvalue() { return Noisy{}; }        // C++17 保证：只有一次构造
Noisy makeNamed()   { Noisy n; return n; }     // NRVO：通常也只有一次构造
Noisy makeBad()     { Noisy n; return std::move(n); }  // 反例！解释见下

int main() {
    std::cout << "-- 返回临时 --\n";   Noisy a = makePrvalue();
    std::cout << "-- 返回具名 --\n";   Noisy b = makeNamed();
    std::cout << "-- 画蛇添足 --\n";   Noisy c = makeBad();
}
```

前两个通常各打印一次「构造」；`makeBad` 会多出一次「移动构造」。

### 8.2 为什么 `return std::move(local);` 是画蛇添足

学完第 7 节最容易犯的错：「返回前 move 一下，帮编译器选移动，多贴心。」恰恰相反：

- NRVO 的触发条件之一是 `return` 后面**直接是那个具名局部变量**；
- `std::move(local)` 是一个右值引用表达式，不是具名变量 → **NRVO 条件被你亲手破坏**；
- 编译器只好退而求其次，老老实实执行一次移动构造。

你把「零次构造」优化成了「一次移动」。这个错误常见到 GCC/Clang 专门为它设了警告（`-Wpessimizing-move`，直译「悲观化的 move」），开着 `-Wall` 就能看到编译器劝你把 `std::move` 删掉。铁律：**返回局部对象，直接 `return local;`，什么都别裹。** 编译器在这件事上比你专业。（`std::move` 在 return 里偶有用武之地——返回成员、返回函数参数这类 NRVO 本来就做不了的场合——但那是特例，先记铁律。）

---

## 9. noexcept：vector 敢不敢用你的移动

第 6 节的移动构造签名里挂着一个还没兑现的 `noexcept`。它不是装饰——**不写它，你的移动构造在最需要发挥的地方会被整个无视**。这件事要从 `std::vector` 扩容说起（vector 是 M4 的老朋友，M7 还会深挖）。

### 9.1 扩容现场的两难

`push_back` 时容量满了，vector 的流程：分配一块更大的内存 → **把旧内存里的 n 个元素搬过去** → 释放旧内存。而 vector 对外承诺了**强异常保证**：`push_back` 如果失败（抛异常），vector 保持原样，一个元素都不丢。

「搬」用拷贝还是移动？推演两条分支：

- **用拷贝搬**：搬到第 k 个时拷贝构造抛了异常 → 旧内存里 n 个元素**原封未动**（拷贝不动源）→ 把新内存里已建好的 k 个副本析构、释放新内存 → vector 完好如初。**可以回滚。**
- **用移动搬**：搬到第 k 个时移动构造抛了异常 → 前 k 个元素的资源已被搬进新内存，**旧内存里它们只剩空壳** → 想回滚就得把 k 个再移回去，而移动既然抛过一次就可能再抛，回滚本身可能失败。**无法可靠回滚，强异常保证碎了。**

死结的解法是把「移动会不会抛」变成编译期可查的事实——这正是 `noexcept` 的本职：一个可被查询的「本函数不抛异常」的承诺。vector 内部用 `std::move_if_noexcept` 做决策：

| 你的移动构造 | vector 扩容搬元素时 |
|---|---|
| `noexcept` | 放心用**移动**，O(1) 每元素 |
| 没标（哪怕实际不会抛） | 不敢赌，退回**拷贝**（有拷贝构造的话） |

### 9.2 结论与验证

**移动构造/移动赋值一律标 `noexcept`。** 对我们这种「偷指针 + 置空」的移动，这个承诺是白给的——抄几个标量成员本来就不可能抛。不标的代价则是隐形的：代码全对、编译全过，vector 扩容时你的移动构造一次都不会被调用，性能默默退回 C++98。

验证方法就在 mini 项目里：给 Rule of 5 的 Buffer 加打印，`push_back` 几个进 `vector<Buffer>` 触发扩容，看打印的是「移动构造」还是「拷贝构造」；再把 `noexcept` 删掉重编译对比一次。亲眼看一遍，胜过背十遍。

---

## 10. 收束：Rule of 5 → Rule of 0，=default / =delete

链条上的零件齐了：拷贝三件套（第 3~4 节）+ 移动两件套（第 6 节）。最后回答工程上真正的问题——**这五个特殊成员函数，到底该写几个？** 答案有两级。

### 10.1 Rule of 5：要写就写全五个

Rule of 3 的逻辑（第 3.1 节：类拥有资源 → 三个函数连坐）在 C++11 加入移动后自然扩展成 **Rule of 5**：析构、拷贝构造、拷贝赋值、移动构造、移动赋值。「连坐」升级还多了一个新理由——**特殊成员函数的自动生成规则是相互纠缠的**：

| 你手写了…… | 拷贝二人组还自动生成吗 | 移动二人组还自动生成吗 |
|---|---|---|
| 什么都没写 | 生成 | 生成 |
| 析构 | 生成（惯例已废弃，别依赖） | **不生成** |
| 拷贝构造或拷贝赋值 | 另一个仍生成（同样别依赖） | **不生成** |
| 移动构造或移动赋值 | **不生成（被隐式 delete）** | 另一个不生成 |

最伤人的是第二、三行：你按 Rule of 3 写了析构和拷贝，编译器就**悄悄不给你生成移动了**。类还能用——所有该移动的场合都静默退化成拷贝（`std::move` 了个寂寞，vector 扩容全程深拷贝）——只是慢，而且不报错。所以规矩是：**五个里手写了任何一个，就把五个全部显式交代清楚**（手写、`=default` 或 `=delete` 都算交代）。

```cpp
class Buffer {                                      // Rule of 5 全家福
public:
    explicit Buffer(std::size_t n);                 // 普通构造
    ~Buffer();                                      // 1 析构
    Buffer(const Buffer& other);                    // 2 拷贝构造
    Buffer& operator=(const Buffer& other);         // 3 拷贝赋值
    Buffer(Buffer&& other) noexcept;                // 4 移动构造
    Buffer& operator=(Buffer&& other) noexcept;     // 5 移动赋值
private:
    std::size_t size_;
    int*        data_;
};
```

### 10.2 Rule of 0：更好的答案是一个都不写

回到第 1 节的种子：**成员全是「懂得拷贝自己」的类型时，默认的逐成员拷贝/移动是完全正确的。** 那就让成员都变成懂事的类型——把裸指针换成 `std::vector`、`std::string`、`std::unique_ptr`（全是 M4 装备），五个特殊成员函数**一个都不写**：

```cpp
#include <iostream>
#include <vector>
#include <cstddef>
#include <utility>

class Buffer {
public:
    explicit Buffer(std::size_t n) : data_(n) {}   // vector 值初始化 n 个 0
    void set(std::size_t i, int v) { data_[i] = v; }
    int  get(std::size_t i) const  { return data_[i]; }
    std::size_t size() const       { return data_.size(); }
    // 五个特殊成员函数：一个都不写！
private:
    std::vector<int> data_;   // Rule of 5 由 vector 实现，我们坐享其成
};

int main() {
    Buffer a(4);
    a.set(0, 42);
    Buffer b = a;              // 默认拷贝 = 逐成员 → vector 深拷贝，正确
    b.set(0, 99);
    std::cout << a.get(0) << " " << b.get(0) << "\n";   // 42 99，互不影响
    Buffer c = std::move(a);   // 默认移动 = 逐成员 → vector 移动，O(1) 且 noexcept
    std::cout << c.get(0) << "\n";
}
```

编译器默认生成的拷贝会逐成员调用 vector 的深拷贝，默认移动逐成员调用 vector 的移动（连 noexcept 都自动推导出来）——第 3~9 节操心的每一件事，vector 的作者都替你写好并测了三十年。这就是 **Rule of 0**：**不直接持有裸资源的类，一个特殊成员函数都不写。**

| | Rule of 5 | Rule of 0 |
|---|---|---|
| 适用 | 你在写资源管理的**底层原语**（自制智能指针、句柄包装） | 其余几乎所有类 |
| 你写的代码 | 5 个函数，处处是第 4/6/9 节的细节 | 0 个函数 |
| 出错面 | 自赋值、置空、noexcept、生成规则…… | 几乎没有 |
| 现代定位 | 特殊工种 | **默认首选** |

学习路径恰好反过来：练习里你要先手写 Rule of 5（不亲手推一遍第 2~9 节，你不会真信 Rule of 0 安全在哪），工程里则默认 Rule of 0。

### 10.3 =default 与 =delete：对生成规则的显式表态

Rule of 5 说「五个都要交代」，`=default` / `=delete` 就是两种最省事的交代方式：

```cpp
class Widget {
public:
    ~Widget();                                    // 手写了析构（比如要打日志）
    Widget(const Widget&)            = default;   // 显式要回默认拷贝
    Widget& operator=(const Widget&) = default;
    Widget(Widget&&) noexcept        = default;   // 要回被析构抑制掉的默认移动
    Widget& operator=(Widget&&) noexcept = default;
};
```

- `=default`：「我要编译器的默认版本，并且是**认真想过后**要的。」典型用途正如上例——手写析构导致移动不再生成（10.1 的表），用 `=default` 一行要回来。
- `=delete`：「这个操作**不允许存在**」，谁调用谁编译错误：

```cpp
class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(const NonCopyable&)            = delete;   // 禁止拷贝
    NonCopyable& operator=(const NonCopyable&) = delete;
};
```

vs C：C 里「这个 struct 不许复制」只能写在注释里求人自觉；`=delete` 把约定变成了编译器强制执行的规则。

### 10.4 兑现 M4 的伏笔：unique_ptr 为什么删除拷贝、保留移动

M4 留过一句话：「`unique_ptr` 不可拷贝，只可移动。」现在你有了完整的推理链，它不再是需要记忆的规定，而是必然的设计：

- `unique_ptr` 的语义是**独占所有权**：一块内存有且只有一个主人;
- 如果允许拷贝 → 两个 unique_ptr 指向同一块内存 → 各析构 delete 一次 → **第 2 节的 double free 原样重演**。所以拷贝构造和拷贝赋值被 `=delete`——不是做不到，是这个语义下拷贝**就该是编译错误**；
- 移动却完全兼容独占：偷指针 + 源置空（第 6 节）正是「所有权交接」的实现——转移前后，主人始终只有一个。

```cpp
#include <memory>
#include <utility>

int main() {
    auto p1 = std::make_unique<int>(42);
    // std::unique_ptr<int> p2 = p1;         // 反例：编译错误，拷贝被 =delete
    std::unique_ptr<int> p3 = std::move(p1); // OK：所有权 p1 → p3，p1 变 nullptr
}
```

一句话收束全模块：**`unique_ptr` = Rule of 5 + 把拷贝二人组 `=delete`**，而你的类只要用它当成员，就自动成为「不可拷贝、可移动」的 Rule of 0 类。M4 的 RAII 讲「资源的生死绑定到对象」，M5 补上了另一半：「资源的**流转**（复制/交接）也绑定到对象」。至此资源管理的故事才算完整。

（顺带一提 `shared_ptr` 的对照：它对「拷贝」给出了另一种回答——拷贝 = 引用计数 +1，最后一个主人析构时才释放。共享所有权是另一种语义，代价是计数开销，M4 讲过的取舍。）

---

## 11. 预告：引用折叠、转发引用与 std::forward（M6 见真章）

最后补一块「见过就好」的拼图，因为它长得和右值引用一模一样，不提前打招呼，M6 你一定会撞上。**简述即止，这里不展开。**

### 11.1 模板里的 `T&&` 不是右值引用

第 5 节说 `Buffer&&` 只绑右值。但**当 `T` 是待推导的模板参数时**，`T&&` 是另一种东西——**转发引用（forwarding reference / universal reference）**，左值右值都能绑：

- 传左值 `int a` → `T` 推导为 `int&` → `T&&` 写出来是 `int& &&`；
- 传右值 `5` → `T` 推导为 `int` → `T&&` 就是 `int&&`。

`int& &&` 这种「引用的引用」按**引用折叠**规则化简，规则只有一句：**参与折叠的只要有左值引用，结果就是左值引用；全是右值引用才折叠出右值引用**（`& &`、`& &&`、`&& &` → `&`；`&& &&` → `&&`）。

### 11.2 std::forward：转发时保住值类别

转发引用的参数**有名字，所以在函数体内是左值**（第 5 节的判据）——直接往下传会把右值「传丢」。`std::forward<T>` 按 `T` 的推导结果把原始值类别还原回去：

```cpp
#include <iostream>
#include <utility>

void process(int&)  { std::cout << "左值版本\n"; }
void process(int&&) { std::cout << "右值版本\n"; }

template <typename T>
void wrapper(T&& arg) {                // 转发引用：左值右值都能进
    process(std::forward<T>(arg));     // 保持实参原本的值类别往下传
}   // 若直接写 process(arg)：arg 有名字是左值，永远命中左值版本

int main() {
    int a = 5;
    wrapper(a);   // T = int&  → 打印 左值版本
    wrapper(5);   // T = int   → 打印 右值版本
}
```

现阶段记三条就够：**① 具体类型的 `&&`（如 `Buffer&&`）是右值引用；② 待推导的 `T&&` 是转发引用；③ 转发引用往下传要配 `std::forward`，就像右值引用配 `std::move`。** 完美转发的完整故事（为什么 `make_unique`、`emplace_back` 靠它实现）放在 M6 模板。

---

## 12. 常见坑

1. **有裸指针成员却用默认拷贝** → 第 2 节的推演原样上演：共享 → double free。有资源就 Rule of 5，能 Rule of 0 更好。
2. **把 `Buffer b = a;` 当成赋值** → 它是初始化，调的是拷贝构造；只有对活着的对象 `=` 才是 `operator=`（第 1.1 节）。
3. **拷贝赋值忘了防自赋值 / 忘了释放旧资源 / 先 delete 后 new** → 第 4 节的三件事，漏哪件坏哪件。记不住三件事就直接上 copy-and-swap，结构上没有可漏的步骤。
4. **移动构造/移动赋值忘了置空源对象** → 源析构时 delete 已交接的内存，double free 换个入口重演（第 6.2 节）。
5. **移动后继续使用源对象的值** → 「有效但未指定」：能析构、能重新赋值，但别读它的旧值（第 6.3 节）。
6. **以为 `std::move` 会搬数据** → 它是零运行期代码的类型转换，只影响重载决议；真搬家的是移动构造/赋值（第 7 节）。
7. **对 const 对象 `std::move`** → 绑不上非 const 的 `T&&`，静默退化为拷贝，无错无警告（第 7.3 节）。打算移动的对象别声明 const。
8. **`return std::move(local);`** → 亲手破坏 NRVO 触发条件，把零开销变成一次移动（第 8.2 节）。返回局部对象什么都别裹。
9. **移动函数没标 `noexcept`** → vector 扩容为保强异常安全退回拷贝，你的移动白写（第 9 节）。
10. **手写了析构/拷贝，忘了移动二人组** → 生成规则表（第 10.1 节）：移动不再自动生成，全类静默退化成只能拷贝。五个要交代就交代全。
11. **给天生独占的东西写拷贝** → 该学 `unique_ptr`：独占语义下拷贝就该 `=delete`，用移动转移所有权（第 10.4 节）。

---

## 13. 高频面试点（附答案要点）

- **浅拷贝和深拷贝的区别？浅拷贝为什么危险？**
  浅拷贝对指针成员只复制地址 → 两对象共享一块内存 → 各自析构时二次释放（double free），此外还有别名修改、悬垂指针问题。深拷贝为副本另行分配内存并复制内容，各管各的。（第 2~3 节的推演能完整讲出来是加分项。）
- **Rule of 3 / 5 / 0 分别是什么？三者关系？**
  Rule of 3：手写析构/拷贝构造/拷贝赋值之一就该三个全写——三者同源于「类拥有资源」。Rule of 5：C++11 加移动构造/移动赋值；且手写任一会抑制其它的自动生成，所以五个要一起交代。Rule of 0：资源交给 vector/智能指针等成员管理，五个全不写，默认逐成员行为即正确——现代首选，Rule of 5 只用于写资源管理底层类。
- **拷贝赋值要注意什么？copy-and-swap 好在哪？**
  四件事：防自赋值、释放旧资源、异常安全（先分配复制后释放）、返回 `*this`。copy-and-swap 按值传参让拷贝发生在进函数前（强异常保证）、交换副本天然自赋值安全、副本析构自动带走旧资源，还顺带兼任移动赋值。
- **什么是左值、右值？怎么判断？**
  左值：有名字、可取地址、表达式结束后仍存活；右值：字面量和临时对象，表达式结束即销毁。判据：能 `&` 的是左值。注意值类别属于表达式而非变量。
- **`std::move` 到底做了什么？**
  零运行期代码，约等于 `static_cast<T&&>`：把左值表达式转成右值，让重载决议选中移动版本。它本身不移动任何东西，也不保证移动发生（如 const 对象会退化为拷贝）。
- **什么情况下会触发移动而不是拷贝？**
  实参是右值（临时对象、`std::move` 的结果）且类有可用的移动构造/赋值；重载决议中 `T&&` 对右值是比 `const T&` 更优的匹配。
- **移动构造为什么要把源对象置空？移动后源对象什么状态？**
  不置空则源析构时 delete 已交接的内存 → double free；置空后源析构执行 `delete nullptr`（标准保证无害）。移动后源对象「有效但未指定」：可析构、可重新赋值，不可依赖其值。
- **移动构造为什么要 `noexcept`？**
  vector 扩容搬元素时要维持强异常保证：拷贝失败可回滚（源未动），移动到一半失败无法回滚（源已被掏空）。故 vector 经 `move_if_noexcept` 只对 noexcept 的移动放行，否则退回拷贝——不标就等于白写。
- **RVO/NRVO 是什么？为什么 `return local;` 不要加 `std::move`？**
  编译器直接在调用方的内存上构造返回对象，消除拷贝/移动；C++17 对返回无名临时是强制的。`std::move(local)` 使返回表达式不再是具名变量，破坏 NRVO 条件，反而强制一次移动。
- **`unique_ptr` 为什么不可拷贝只可移动？**
  独占所有权语义：拷贝会造成两个 owner → double free，故拷贝二人组被 `=delete`；移动的「偷指针+置空」恰好实现所有权转移，主人始终唯一。
- **`=default` / `=delete` 的用途？**
  `=default` 显式要回默认实现（典型：手写析构后要回被抑制的移动）；`=delete` 让某操作成为编译错误（典型：禁止拷贝，实现独占/单例语义）。
- **模板里的 `T&&` 是右值引用吗？**
  不是，是转发引用：左值右值都能绑，靠引用折叠（有 `&` 则 `&`）实现，配 `std::forward` 保持值类别转发。具体类型的 `&&` 才是右值引用。

---

## 14. 编译提醒

单文件练习（x64 Native Tools 命令行）：

```
cl /EHsc /std:c++17 /W4 文件名.cpp
```

mini 项目若拆成多文件：

```
cl /EHsc /std:c++17 /W4 main.cpp Buffer.cpp
```

两个本模块专属提醒：

- **每个特殊成员函数都加打印**（带 `data_` 地址更好），然后先在纸上预测输出、再运行对答案——这是理解拷贝/移动最快的路，也是 mini 项目的核心玩法。
- 观察 RVO 时注意：`cl` 不开优化也执行 C++17 强制消除，但 NRVO 在 `/Od`（默认 Debug）下可能不做；对比 `/O2` 的输出会很有启发。

---

## 15. 承前启后

- **承前（M2）**：拷贝/移动构造是构造函数的重载，析构还是那个析构，`return *this` 在两种赋值里再次登场；「局部对象逆序析构」正是第 2 节推演 double free 的关键一环。
- **承前（M4）**：RAII 说「资源的生死绑定到对象」，本模块补完「资源的流转也绑定到对象」；M4 埋的「`unique_ptr` 不可拷贝只可移动」在第 10.4 节兑现——`=delete` 拷贝 + 移动转移所有权就是独占语义的实现。Rule of 0 的底气正是 M4 那批实现好 Rule of 5 的类型。
- **启后（M6 模板）**：第 11 节的转发引用和 `std::forward` 会在模板里正式展开——记住那三条：具体类型的 `&&` 是右值引用，待推导的 `T&&` 是转发引用，转发配 forward。
- **启后（M7 容器与算法）**：`push_back(x)` 拷贝、`push_back(std::move(x))` 移动、扩容搬移看 noexcept（第 9 节），以及 `emplace_back` 如何靠完美转发连临时对象都省掉——到时候这些都不是新知识，只是本模块结论在容器上的应用。

下一步：打开 `exercises.md`。前四题把拷贝三件套亲手写对（练习 2 亲眼看浅拷贝的地址共享），练习 5 建立值类别直觉，练习 6 补齐移动凑成 Rule of 5，练习 7~8 走 `=default`/`=delete` 和 Rule of 0，最后 mini 项目把六个函数全部实现、用打印观察每一次调用——做到能**预测**每一行输出，这个模块才算真正到手。
