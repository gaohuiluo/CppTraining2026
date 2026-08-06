# M4 内存管理与 RAII（完整）

> 目标：把 C++ 的内存管理彻底理清。你在 C 里靠 `malloc`/`free` 手动管理堆内存，一不小心就泄漏、悬垂、double free。C++ 一方面给了你 `new`/`delete`（带构造/析构），另一方面用 **RAII** 把资源生命周期绑到对象上，再用**智能指针**把「谁该释放」这件事交给编译器自动处理。这一模块讲全：栈 vs 堆、`new`/`delete`、内存错误、RAII、`unique_ptr`/`shared_ptr`/`weak_ptr`、所有权模型。

---

## 0. 一句话总览

**内存管理的终极目标：每一块资源都有明确的「拥有者」，拥有者消失时资源自动释放。**
C 靠你手动记着 `free`；C++ 靠 RAII + 智能指针让「释放」自动发生。现代 C++ 的准则是：**几乎不再手写 `new`/`delete`。**

M2 里你已经建立了直觉：**析构 = 自动清理**。M4 就是把这个直觉推到极致——用栈对象的析构去管理堆资源。

---

## 1. 栈内存 vs 堆内存

程序运行时，内存主要分两块用途（对 C 老手不陌生，先对齐术语）：

| | 栈（stack） | 堆（heap） |
|---|---|---|
| C++ 术语 | 自动存储期（automatic） | 动态存储期（dynamic） |
| 谁分配/释放 | 编译器自动 | 你手动（`new`/`delete`）|
| 生命周期 | 离开作用域自动销毁 | 直到你 `delete` 才销毁 |
| 大小 | 编译期基本已知，容量有限（MB 级）| 运行期决定，容量大 |
| 速度 | 快（就是移动栈指针）| 慢（要找空闲块）|
| C 对应 | 局部变量 | `malloc` 的内存 |

```cpp
void demo() {
    int a = 10;             // 栈：a 随函数结束自动消失
    int* p = new int(20);   // 堆：*p 一直活着，直到 delete
    // ...
    delete p;               // 必须手动释放，否则泄漏
}   // a 自动销毁；如果忘了 delete p，*p 就泄漏了
```

关键区别：**栈对象离开作用域会自动调用析构函数**，堆对象只有在你 `delete` 时才会调用析构函数。RAII 的全部魔法都建立在「栈对象析构自动触发」这一点上。

> 什么时候必须用堆？
> - 对象大小/数量在运行期才确定（如用户输入决定的数组长度）。
> - 对象生命周期要**超出**创建它的作用域（工厂函数返回一个对象）。
> - 对象很大，放栈上有溢出风险。
> 其它情况优先用栈对象——更快、更安全、不用操心释放。

---

## 2. `new` / `delete` vs C 的 `malloc` / `free`

C 里你这么分配一个对象：

```c
Widget* p = (Widget*)malloc(sizeof(Widget));   // 只分配内存，不初始化
if (!p) { /* 处理失败 */ }
// ... 得手动初始化 p 的字段 ...
free(p);                                        // 只释放内存，不调用任何清理逻辑
```

C++ 用 `new` / `delete`：

```cpp
Widget* p = new Widget(42);   // 1) 分配内存 2) 调用构造函数
// ...
delete p;                     // 1) 调用析构函数 2) 释放内存
```

逐条对比：

| | `malloc` / `free` | `new` / `delete` |
|---|---|---|
| 构造/析构 | **不调用**，只管内存 | `new` 调构造，`delete` 调析构 |
| 类型 | 返回 `void*`，要强转 | 返回正确类型指针，无需强转 |
| 大小 | 要自己写 `sizeof(T)` | 编译器自动算，不用写 sizeof |
| 分配失败 | 返回 `NULL`，要检查 | 默认**抛 `std::bad_alloc` 异常**，不返回空 |
| 头文件 | `<cstdlib>` | 语言内置关键字 |

最本质的差别就是那句：**`new`/`delete` 会调用构造/析构函数**。对有资源的类（比如内部还 `new` 了东西的类），用 `malloc` 就是灾难——构造函数没跑，对象根本没初始化好。

> 铁律：**`new` 配 `delete`，`malloc` 配 `free`，绝不混用。** `malloc` 出来的内存用 `delete` 释放、或 `new` 出来的用 `free` 释放，都是未定义行为。

---

## 3. `new[]` / `delete[]`：数组版本

分配一个对象数组要用 `new[]`，释放要用配套的 `delete[]`：

```cpp
int* arr = new int[100];      // 分配 100 个 int
// ...
delete[] arr;                 // 注意是 delete[]，不是 delete

Widget* ws = new Widget[10];  // 分配并逐个构造 10 个 Widget
delete[] ws;                  // 逐个析构后释放
```

⚠️ **`new[]` 必须配 `delete[]`，`new` 必须配 `delete`。** 用错版本是未定义行为：

```cpp
int* arr = new int[100];
delete arr;      // 错！应该 delete[]，可能只释放部分/破坏堆
```

为什么会错？`new[]` 通常会在分配的内存前面额外记录「元素个数」，`delete[]` 靠它知道要析构几个对象。用 `delete` 释放 `new[]` 的内存，这个记账信息对不上，行为未定义。

> 现代 C++ 的态度：**别手写 `new[]`/`delete[]` 了**，用 `std::vector`（M6 讲容器）或 `std::make_unique<T[]>(n)`。裸数组的手动管理是纯粹的坑。

---

## 4. 常见内存错误（C 老手最熟悉的痛）

这些错误在 C 里天天见，C++ 如果还手写裸指针，一样会犯。逐个上例子。

### 4.1 内存泄漏（memory leak）
分配了不释放，内存一直被占用直到程序结束。

```cpp
void leak() {
    int* p = new int[1000];
    // ... 用完忘了 delete[] p ...
}   // p 这个栈指针没了，但它指向的堆内存还在——再也没人能释放它了
```

隐蔽版本：函数中途 `return` 或抛异常，跳过了 `delete`：

```cpp
void tricky() {
    int* p = new int(1);
    if (some_condition()) return;   // 提前返回，泄漏！
    delete p;
}
```

### 4.2 悬垂指针（dangling pointer）
指针指向的内存已经被释放，但还在用它。

```cpp
int* p = new int(42);
delete p;            // 内存还给系统了
std::cout << *p;     // 悬垂！*p 是已释放内存，未定义行为
p = nullptr;         // 好习惯：delete 后置空，避免误用
```

### 4.3 重复释放（double free）
同一块内存 `delete` 两次。

```cpp
int* p = new int(42);
delete p;
delete p;            // double free！堆管理结构被破坏，通常直接崩溃
```

常见于两个指针指向同一块内存，各自都 `delete`：

```cpp
int* a = new int(1);
int* b = a;          // b 和 a 指向同一块内存
delete a;
delete b;            // double free！其实是同一块
```

### 4.4 `new` / `delete[]` 不匹配
上一节讲过，`new` 配 `delete`，`new[]` 配 `delete[]`，混用是未定义行为。

```cpp
Widget* w = new Widget[5];
delete w;            // 错！只析构了第一个（或更糟），应该 delete[]
```

> 这四类错误，正是智能指针要根治的目标。看完下面的 RAII 和智能指针，你会发现：**当所有权清晰、释放自动化后，这些错误从根上消失了。**

---

## 5. RAII：C++ 最重要的思想

RAII = **Resource Acquisition Is Initialization**（资源获取即初始化）。名字很拗口，核心其实一句话：

**把资源的「获取」放进构造函数，把资源的「释放」放进析构函数。这样资源的生命周期 = 对象的生命周期，栈对象离开作用域时自动释放资源。**

### 5.1 C 的痛：手动配对

C 里你要手动保证「获取」和「释放」成对出现：

```c
FILE* fp = fopen("data.txt", "r");   // 获取
if (!fp) return;
// ... 一堆处理，中间可能 return、可能出错 ...
fclose(fp);                          // 释放——你必须记得，且每条退出路径都要有
```

痛点：
- 每一条 `return` / `break` / 错误分支前，都得记得 `fclose`。漏一条就泄漏。
- 代码越长、分支越多，越容易漏。
- `malloc`/`free`、`lock`/`unlock` 同理，全靠人肉配对。

### 5.2 C++ 的解法：让析构函数替你释放

把 `FILE*` 包进一个类，构造时 `fopen`，析构时 `fclose`：

```cpp
class File {
public:
    explicit File(const char* path) : fp_(std::fopen(path, "r")) {}
    ~File() { if (fp_) std::fclose(fp_); }    // 析构自动关，你不用管
    std::FILE* get() const { return fp_; }
private:
    std::FILE* fp_;
};

void process() {
    File f("data.txt");     // 构造：打开
    if (something()) return;   // 提前返回也没事——f 析构会自动 fclose
    // ... 用 f.get() ...
}   // 无论从哪条路径离开，f 析构都会 fclose。零泄漏。
```

对比感受一下：
- C 版本：每条退出路径手动 `fclose`，漏一条就泄漏。
- C++ 版本：**只在构造/析构里写一次**，之后所有退出路径（正常返回、提前返回、抛异常）都自动释放。

> 这就是 RAII 的威力：**把「记得释放」这件事，从「程序员的纪律」变成「编译器的保证」。** 异常安全也顺带解决了——即使中间抛异常，栈展开时局部对象的析构照样会被调用。

标准库里所有管理资源的类型都是 RAII：`std::string`、`std::vector`、`std::fstream`、`std::lock_guard`、以及下面的智能指针。你几乎不用自己写 RAII 类，直接用它们就行。

---

## 6. 智能指针总览

智能指针是「长得像指针、但会自动管理内存」的 RAII 类模板，定义在头文件 `<memory>`。它们把「堆对象的所有权」用类型表达出来，析构时自动 `delete`。

C++17 常用三种：

| 智能指针 | 所有权 | 能否拷贝 | 典型场景 |
|---|---|---|---|
| `std::unique_ptr` | 独占（唯一拥有者）| 不可拷贝，可移动 | 默认首选，绝大多数场景 |
| `std::shared_ptr` | 共享（引用计数）| 可拷贝 | 多个拥有者，生命周期不确定 |
| `std::weak_ptr` | 不拥有（观察者）| 可拷贝 | 打破 `shared_ptr` 循环引用 |

核心心法：**先问「这块内存谁拥有」，答案决定用哪种指针。** 独占用 `unique_ptr`，共享用 `shared_ptr`，只观察不拥有用 `weak_ptr` 或裸指针。

---

## 7. `std::unique_ptr`：独占所有权

`unique_ptr` 表示「我是这块内存的**唯一**拥有者」，它销毁时自动 `delete`。这是你**默认应该用的**智能指针。

```cpp
#include <memory>

void demo() {
    std::unique_ptr<Widget> p = std::make_unique<Widget>(42);   // 推荐这样创建
    p->doSomething();        // 像裸指针一样用 -> 和 *
    (*p).doSomething();
}   // p 离开作用域，自动 delete，Widget 的析构被调用。不用手写 delete。
```

### 7.1 优先用 `std::make_unique`
C++14 起有 `std::make_unique`，比 `new` 更好：

```cpp
auto p = std::make_unique<Widget>(42);      // 推荐
std::unique_ptr<Widget> q(new Widget(42));  // 也行，但不如上面
```

好处：代码里**看不到裸 `new`**，也就不会有「`new` 了忘记交给智能指针」的窗口期，还能少写一遍类型名。

### 7.2 不可拷贝，只能移动
`unique_ptr` 的「独占」是靠**禁止拷贝**实现的——如果能拷贝，就有两个拥有者了，违背独占。

```cpp
auto a = std::make_unique<Widget>(1);
auto b = a;              // 编译错误！unique_ptr 不能拷贝
auto c = std::move(a);   // OK：移动。所有权从 a 转给 c，之后 a 变成空(nullptr)
```

`std::move` 把所有权「转移」出去，转移后原来的 `unique_ptr` 变成空。这就是「移动语义」的一个入口——**移动的完整故事（拷贝 vs 移动、Rule of 0/3/5）是 M5 的核心，这里你只要记住：`unique_ptr` 独占、不可拷贝、可移动。**

### 7.3 从函数返回 unique_ptr（工厂模式）
```cpp
std::unique_ptr<Widget> makeWidget(int id) {
    return std::make_unique<Widget>(id);   // 直接返回，所有权转移给调用者
}
auto w = makeWidget(7);   // w 现在拥有这个 Widget
```

这是 `unique_ptr` 最漂亮的用法：把「工厂函数返回堆对象」这件事做得既安全又零成本，调用者拿到就自动负责释放。

### 7.4 常用操作
```cpp
auto p = std::make_unique<int>(5);
int* raw = p.get();      // 拿到裸指针(只借用，别 delete 它)
p.reset();               // 提前释放，p 变 nullptr
p.reset(new int(9));     // 释放旧的，接管新的
int* r = p.release();    // 放弃所有权，返回裸指针——现在你得自己 delete r！
if (p) { /* p 非空 */ }  // 可直接当 bool 判空
```

---

## 8. `std::shared_ptr`：共享所有权

有时一块资源**没有唯一拥有者**——多个对象都要用它，谁都可能是最后一个用完的。这时用 `shared_ptr`：它内部维护一个**引用计数**，记录「有多少个 `shared_ptr` 指向这块内存」。计数归零时才 `delete`。

```cpp
#include <memory>

auto a = std::make_shared<Widget>(42);   // 引用计数 = 1
{
    auto b = a;          // 拷贝：计数 = 2，a 和 b 指向同一个 Widget
    std::cout << a.use_count();   // 2
}                        // b 销毁：计数 = 1（还没到 0，不 delete）
// 这里计数还是 1
// a 销毁时计数 = 0 -> 真正 delete Widget
```

要点：
- **可以自由拷贝**，每次拷贝计数 +1，每次销毁计数 -1。
- 计数归 0 时自动 `delete`。你永远不用手动 delete。
- `use_count()` 能查当前计数（调试用）。

### 8.1 优先用 `std::make_shared`
```cpp
auto p = std::make_shared<Widget>(42);      // 推荐
std::shared_ptr<Widget> q(new Widget(42));  // 也行，但不如上面
```

`make_shared` 相比 `new` 的好处（**高频面试点**）：
1. **一次分配**：把「对象本身」和「引用计数控制块」合并成一次堆分配（`new` 版本是两次：一次给对象，一次给控制块）。更快、内存更紧凑。
2. **异常安全**：没有裸 `new` 暴露在外，不会出现「对象 new 好了但还没交给 shared_ptr 就抛异常」的泄漏窗口。
3. 代码更短，类型名只写一遍。

> 一个小权衡：`make_shared` 把对象和控制块放一起，只要还有 `weak_ptr` 存活，整块内存（含对象占的空间）就不能释放。绝大多数情况这不是问题，优先 `make_shared`。

### 8.2 shared_ptr 的开销
`shared_ptr` 比 `unique_ptr` 重：多一个控制块、计数的增减是**原子操作**（为了线程安全）。所以**别无脑用 shared_ptr**——能用 `unique_ptr` 就用它，确实需要共享所有权才上 `shared_ptr`。

---

## 9. `std::weak_ptr`：打破循环引用

`shared_ptr` 有个致命陷阱：**循环引用**。两个对象用 `shared_ptr` 互相指向对方，它们的引用计数永远降不到 0，谁都不会被释放——内存泄漏。

### 9.1 循环引用泄漏的例子
```cpp
struct Node {
    std::shared_ptr<Node> next;   // 都用 shared_ptr 互指
    std::shared_ptr<Node> prev;
    ~Node() { std::cout << "Node 析构\n"; }
};

void leak() {
    auto a = std::make_shared<Node>();   // a 的计数 1
    auto b = std::make_shared<Node>();   // b 的计数 1
    a->next = b;   // b 的计数 2
    b->prev = a;   // a 的计数 2
}   // 离开作用域：局部变量 a、b 销毁，各自计数从 2 减到 1
    // 但 a->next 还拿着 b(计数1)，b->prev 还拿着 a(计数1)
    // 谁都到不了 0 -> 两个 Node 都不析构 -> 泄漏！(看不到 "Node 析构" 打印)
```

问题的本质：`a` 和 `b` 通过成员互相「拥有」对方，形成一个谁也不肯先松手的死锁。

### 9.2 用 weak_ptr 修复
把其中一个方向改成 `weak_ptr`。`weak_ptr` **观察但不拥有**——它不增加引用计数，只是「弱引用」：

```cpp
struct Node {
    std::shared_ptr<Node> next;   // 一个方向仍用 shared(拥有)
    std::weak_ptr<Node>   prev;   // 另一个方向用 weak(不拥有，不加计数)
    ~Node() { std::cout << "Node 析构\n"; }
};

void fixed() {
    auto a = std::make_shared<Node>();
    auto b = std::make_shared<Node>();
    a->next = b;   // b 计数 2
    b->prev = a;   // weak_ptr 不加计数，a 计数仍是 1
}   // 离开作用域：a 计数 1->0 -> 析构；连带 a->next 释放，b 计数 2->1->0 -> 析构。都释放了！
```

### 9.3 用 weak_ptr 要先「升级」
`weak_ptr` 不能直接解引用（它指向的对象可能已经没了）。用之前先调 `lock()` 升级成 `shared_ptr`：

```cpp
std::weak_ptr<Node> w = somePtr;
if (auto sp = w.lock()) {   // lock() 返回 shared_ptr；对象还活着则非空
    sp->doSomething();      // 安全使用
} else {
    // 对象已经被销毁了
}
```

> 经验法则：**表达「拥有」用 `shared_ptr`，表达「引用回去 / 观察者 / 缓存」用 `weak_ptr`。** 父子/双向关系里，通常「父拥有子」用 shared，「子指回父」用 weak。

---

## 10. 自定义删除器（简单了解）

默认智能指针用 `delete` 释放。但有些资源不是用 `delete` 释放的（比如 `FILE*` 要 `fclose`、C 库句柄要专门的释放函数）。你可以给智能指针指定**自定义删除器**：

```cpp
// 让 unique_ptr 管理 FILE*，析构时调用 fclose 而不是 delete
auto closer = [](std::FILE* f){ if (f) std::fclose(f); };
std::unique_ptr<std::FILE, decltype(closer)> fp(std::fopen("data.txt", "r"), closer);
// fp 离开作用域自动 fclose，不用手动关

// shared_ptr 更简单，删除器不进类型
std::shared_ptr<std::FILE> sp(std::fopen("x.txt", "r"),
                              [](std::FILE* f){ if (f) std::fclose(f); });
```

先知道「智能指针能管理任意资源，不只是 `new` 出来的内存」就够了。实战中封装 C 库句柄时很有用。

---

## 11. 所有权模型：贯穿全模块的思维方式

内存管理的一切都可以归结为一个问题：**这块资源，谁拥有它？谁负责释放它？**

理清所有权的思考步骤：
1. **有唯一的拥有者吗？** 有 → `unique_ptr`。这是默认答案，覆盖绝大多数情况。
2. **需要多个拥有者共享，且不确定谁最后用完？** → `shared_ptr`。
3. **只是想「看一眼 / 引用」，不参与拥有？** → 裸指针 `T*` 或引用 `T&`（借用），或 `weak_ptr`（当被观察对象是 shared 管理的、且可能提前消失时）。

关键区分「拥有」和「借用」：
- **拥有（owning）**：我负责它的生命周期，我消失时它也该消失（或计数减一）。用智能指针。
- **借用（non-owning）**：我只是用一下，不负责释放。用裸指针或引用。

```cpp
// 函数参数怎么选（体现所有权意图）：
void observe(const Widget& w);              // 只用一下，不拥有 -> 引用
void observe(const Widget* w);              // 同上，可能为空 -> 裸指针
void take(std::unique_ptr<Widget> w);       // 我要接管所有权 -> 按值传 unique_ptr
void share(std::shared_ptr<Widget> w);      // 我要共享所有权 -> 按值传 shared_ptr
```

> 传参小贴士：**只是用一下对象，就传引用/裸指针，别传智能指针。** 传 `unique_ptr`（按值）意味着「转移所有权」，传 `shared_ptr`（按值）意味着「共享所有权、计数+1」。别在不需要转移/共享所有权时传智能指针，那是滥用。

---

## 12. 何时用哪种指针：决策指引

一张速查表：

| 情况 | 用什么 |
|---|---|
| 对象能放栈上、生命周期不超出作用域 | **直接用栈对象**，别上堆 |
| 堆对象，单一拥有者（绝大多数）| `std::unique_ptr` |
| 堆对象，多个拥有者共享生命周期 | `std::shared_ptr` |
| 打破 shared_ptr 循环引用 / 观察者 | `std::weak_ptr` |
| 只借用、不拥有、不管释放 | 裸指针 `T*` 或引用 `T&` |
| 动态数组 | `std::vector`（首选）或 `make_unique<T[]>(n)` |

决策顺序（从上往下问）：
1. 能用栈对象吗？能就用栈对象。
2. 必须上堆，有唯一拥有者吗？→ `unique_ptr`。
3. 真的需要共享所有权吗？→ `shared_ptr`（+ 必要时 `weak_ptr` 破环）。
4. 只是传进函数用一下？→ 引用或裸指针。

**裸指针在现代 C++ 里没有消失**，但它的角色变了：**裸指针表示「借用/观察」，不表示「拥有」。** 拥有权一律交给智能指针和 RAII。

---

## 13. 常见坑（从 C 过来 + 智能指针新手最易踩的）

1. **`new`/`delete` 与 `new[]`/`delete[]` 混用** → 未定义行为。数组一定 `delete[]`。
2. **`new` 的内存用 `free`、`malloc` 的用 `delete`** → 未定义行为。绝不混用两套。
3. **同一个裸指针交给两个智能指针**：
   ```cpp
   Widget* raw = new Widget;
   std::shared_ptr<Widget> a(raw);
   std::shared_ptr<Widget> b(raw);   // 坑！a、b 各自建了独立的控制块
   // a、b 各自计数为 1，销毁时会 double free！
   ```
   正确做法：用 `make_shared`，或让 `b = a` 拷贝而不是从裸指针重建。
4. **用裸指针接管 `make_unique`/`make_shared` 的对象**：
   ```cpp
   Widget* raw = std::make_unique<Widget>().get();   // 坑！
   // 临时的 unique_ptr 立刻析构，raw 当场变悬垂指针
   ```
5. **把栈对象的地址塞进智能指针**：
   ```cpp
   Widget w;
   std::unique_ptr<Widget> p(&w);   // 坑！p 析构时会 delete 一个栈对象 -> 崩溃
   ```
   智能指针只能管理堆对象（`new` 出来的）。
6. **`shared_ptr` 循环引用** → 计数永不归零，泄漏。用 `weak_ptr` 破环（见第 9 节）。
7. **`release()` 后忘记 delete**：`p.release()` 放弃所有权并返回裸指针，此后释放责任回到你手上，忘了就泄漏。（想「只看不放弃」用 `get()`。）
8. **无脑用 `shared_ptr`**：它有原子计数开销，能用 `unique_ptr` 就别用 `shared_ptr`。
9. **`delete` 后不置空**，之后误用成悬垂指针。手动管理时养成 `delete p; p = nullptr;` 的习惯（智能指针自动规避了这点）。

---

## 14. 高频点（M4 相关）

- **`new`/`delete` 和 `malloc`/`free` 的区别？**（是否调用构造/析构、类型安全、返回值、失败处理）
- **`unique_ptr` 和 `shared_ptr` 的区别？**（独占 vs 共享；不可拷贝只可移动 vs 可拷贝有引用计数；开销）
- **`shared_ptr` 是线程安全的吗？**
  - **引用计数的增减是线程安全的**（原子操作），多个线程各自拷贝/销毁同一个 shared_ptr 指向的对象是安全的。
  - **但它指向的对象本身不是线程安全的**——多线程同时读写被管理的对象，仍要你自己加锁。
  - 而且**同一个 `shared_ptr` 实例**被多个线程同时改（比如同时 reset）也不安全；线程安全指的是「指向同一对象的不同 shared_ptr 实例」的计数操作。
- **什么是循环引用？怎么解决？**（两个 shared_ptr 互指导致计数不归零；用 weak_ptr 打破）
- **`make_shared` 相比 `new` 的好处？**（一次分配、异常安全、代码简洁；权衡：weak_ptr 存活时内存不释放）
- **`weak_ptr` 有什么用？怎么用？**（不拥有、不加计数、破环；用前 `lock()` 升级成 shared_ptr）
- **什么是 RAII？** （资源获取即初始化，用对象生命周期管理资源，析构自动释放；异常安全）
- **`unique_ptr` 为什么不能拷贝？**（独占语义，拷贝会导致两个拥有者、double free；只能 move 转移所有权）
- **智能指针放头文件？**（`<memory>`）

---

## 15. 编译提醒

单文件练习（x64 Native Tools 命令行）：
```
cl /EHsc /std:c++17 /W4 文件名.cpp
```
多文件（头文件+源文件）：
```
cl /EHsc /std:c++17 /W4 main.cpp Other.cpp
```

> 本模块 mini 项目会让你**手写一个简化版 `unique_ptr`**，亲手实现「析构自动释放 + 禁止拷贝 + 支持移动」，你会彻底理解智能指针的原理。

---

## 16. 与相邻模块的呼应

- **回顾 M2**：M2 建立了「析构 = 自动清理」的直觉，还写过一个 `FileWrapper`（构造 fopen、析构 fclose）——那其实就是你的第一个 RAII 类。M4 把它系统化了。
- **回顾 M3**：多态基类记得写 `virtual ~Base()`（虚析构），否则通过基类指针 `delete` 派生对象时析构不完整、会泄漏。`unique_ptr<Base>` 管理派生对象时，这一点同样重要。
- **埋伏笔 M5**：本模块反复提到 `unique_ptr`「不可拷贝、只可移动」，但**「拷贝一个对象到底会发生什么、移动又是什么、什么时候该自己写拷贝/移动函数（Rule of 0/3/5）」是 M5 的核心**。你现在只要建立直觉：独占资源的类型（像 unique_ptr）只能移动不能拷贝。到了 M5，你会亲手实现这些语义，理解 `std::move` 背后到底发生了什么。

---

下一步：打开 `exercises.md`。这一模块的练习会让你把裸 `new`/`delete` 的坑亲手踩一遍，再用智能指针一个个填平，最后手写一个简化 `unique_ptr` 收官。
