# M7 STL 实战（完整）

> 目标：把 STL 用顺手。你在 C 里写业务，一半时间在手搓链表、动态数组、查找、排序、哈希表——这些"数据结构 + 算法"的轮子。C++ 标准库（STL）把它们全给你造好了，而且是**泛型**的（对任意类型都能用，这正是 M6 模板的成果）。这一模块讲全：三大件（容器/迭代器/算法）、各种容器怎么选、迭代器失效这个大坑、常用算法、lambda、`std::function`，以及"容器 + 算法 + lambda"的现代写法。

---

## 0. 一句话总览

**STL = 现成的容器（装数据）+ 迭代器（统一的遍历方式）+ 算法（对数据做操作），三者靠迭代器解耦。**
你不再手写链表和 qsort，而是 `std::vector` + `std::sort` + 一个 lambda 搞定。

---

## 1. 三大件与"解耦"这件事

STL 的核心设计有三部分：

| 组件 | 是什么 | C 里的对应 |
|---|---|---|
| 容器 container | 装数据的模板类，如 `vector`/`map` | 你手写的数组、链表、哈希表 |
| 迭代器 iterator | 指向容器元素的"广义指针" | 裸指针 `int*` 遍历数组 |
| 算法 algorithm | 操作数据的模板函数，如 `sort`/`find` | 你手写的 `qsort`、查找循环 |

关键在**解耦**：算法不直接依赖容器，而是通过迭代器工作。`std::sort` 不知道你传的是 vector 还是 array，它只要一对"起点/终点"迭代器。这样 M 个容器 × N 个算法，不用写 M×N 份代码，各写一份就能自由组合。

```cpp
#include <vector>
#include <algorithm>
#include <iostream>

int main() {
    std::vector<int> v{5, 2, 8, 1, 9};
    std::sort(v.begin(), v.end());          // 算法 + 容器给的迭代器
    for (int x : v) std::cout << x << ' ';  // 1 2 5 8 9
}
```

对比 C 的 `qsort`：你得传数组指针、元素个数、元素大小、还有一个 `int(*)(const void*, const void*)` 的比较函数，函数里还要 `void*` 强转。`std::sort` 全程有类型、无强转、还更快（因为是模板，能内联比较逻辑，不像函数指针每次都要间接调用）。

---

## 2. 序列容器（按插入顺序线性排列）

### 2.1 `std::vector`——默认首选（重点）

动态数组，元素在内存里**连续**。它就是你在 C 里反复手写的"能自动扩容的 malloc 数组"，但不用你管内存。

```cpp
#include <vector>

std::vector<int> v;          // 空
v.push_back(10);             // 尾部追加
v.push_back(20);
std::cout << v[0];           // 下标访问，和 C 数组一样
std::cout << v.size();       // 元素个数
```

**扩容机制（面试高频）**——理解 `size` 和 `capacity` 的区别：

- `size()`：当前**有多少个**元素。
- `capacity()`：当前这块内存**能装多少个**（不重新分配的前提下）。
- `push_back` 时若 `size == capacity`，vector 会：申请一块**更大**的新内存（MSVC/libstdc++ 一般按 1.5～2 倍增长）→ 把老元素搬过去 → 释放老内存。这次 push_back 就"贵"了；但因为是成倍增长，平摊下来每次 push_back 仍是 O(1)（摊还复杂度）。

```cpp
std::vector<int> v;
for (int i = 0; i < 10; ++i) {
    v.push_back(i);
    std::cout << "size=" << v.size() << " cap=" << v.capacity() << '\n';
    // cap 会看到 0→1→2→4→8→16 这种成倍跳变
}
```

**为什么成倍增长而不是每次 +1？** 如果每次只 +1，插入 n 个元素就要搬 1+2+...+n ≈ O(n²) 次。成倍增长让搬运总次数是 O(n)，平摊 O(1)。这就是"为什么两倍"的答案。

**`reserve`——已知规模就先占好，避免反复搬运：**

```cpp
std::vector<int> v;
v.reserve(1000);          // 一次性把 capacity 提到 1000
for (int i = 0; i < 1000; ++i) v.push_back(i);   // 全程不再扩容/搬运
```

> `reserve(n)` 只改 capacity 不改 size；`resize(n)` 改 size（真的多出 n 个元素，值初始化）。别混。

**`emplace_back` vs `push_back`（面试点）：**

```cpp
std::vector<std::string> v;
v.push_back(std::string("hi"));   // 先造临时 string，再移动/拷贝进去
v.emplace_back("hi");             // 直接在容器内存里用 "hi" 构造，省掉临时对象
```

`emplace_back` 把参数**转发**给元素的构造函数，在容器内部就地构造，省去临时对象。元素是类类型时更划算；对 `int` 这种没差别。习惯用 `emplace_back` 不吃亏。

### 2.2 `std::array`——定长，栈上

编译期就定长的数组，是 C 原生数组 `int a[10]` 的安全替代：知道 `size()`、能用迭代器、能整体拷贝、越界有 `at()` 检查。

```cpp
#include <array>
std::array<int, 4> a{1, 2, 3, 4};   // 大小是类型的一部分
std::cout << a.size();               // 4，编译期就知道
```

没有动态扩容，也不额外分配堆内存。大小固定且已知时用它，比 vector 省一次堆分配。

### 2.3 `std::deque`——双端队列

两头都能高效增删（`push_front`/`push_back` 都是 O(1)）。内存**分段连续**（不是整体连续）。需要在头部频繁插入时用它，否则默认还是 vector。

### 2.4 `std::list`——双向链表

就是你在 C 里手写的双向链表，只是不用自己 `malloc` 节点、接指针了。

```cpp
#include <list>
std::list<int> lst{1, 2, 3};
lst.push_front(0);      // 头插 O(1)
```

**什么时候用 list？**——其实很少。只有在"频繁在中间插入/删除，且已经拿到位置迭代器"时它才有优势（改指针即可，不搬元素）。代价是：**不支持下标随机访问**（要遍历），且每个元素单独分配、缓存不友好。现实中 vector 因为内存连续、缓存命中高，即使中间插入要搬元素，也常常比 list 快。**默认别用 list。**

**vector vs list（面试高频）：**

| | vector | list |
|---|---|---|
| 内存 | 连续 | 节点分散，每个带前后指针 |
| 随机访问 `v[i]` | O(1) | 不支持（要 O(n) 遍历） |
| 尾部增删 | 摊还 O(1) | O(1) |
| 中间增删 | O(n)（要搬后面的元素） | O(1)（但前提是已有该位置迭代器） |
| 缓存友好 | 好 | 差 |
| 迭代器失效 | 扩容/增删后大面积失效 | 只有被删的那个失效 |

---

## 3. 关联容器（按 key 组织，快速查找）

这类容器解决的是 C 里最烦的"按 key 查值"——在 C 里你得手写哈希表或有序数组 + 二分。

### 3.1 有序：`std::map` / `std::set`（红黑树）

底层是**红黑树**（一种自平衡二叉搜索树），元素**始终按 key 有序**，增删查都是 **O(log n)**。

- `set`：只存 key 的有序集合（自动去重）。
- `map`：存 key→value 的有序字典。

```cpp
#include <map>
#include <set>

std::map<std::string, int> age;
age["Tom"] = 20;                 // 插入或修改
age["Jerry"] = 18;
std::cout << age["Tom"];         // 查

for (const auto& [name, a] : age)      // C++17 结构化绑定，遍历天然按 key 有序
    std::cout << name << ":" << a << '\n';   // Jerry 先，Tom 后（字典序）

std::set<int> s{3, 1, 2, 1};     // 存成 {1, 2, 3}，重复的 1 被去掉
```

### 3.2 无序：`std::unordered_map` / `std::unordered_set`（哈希表）

底层是**哈希表**，就是你在 C 里手写的那种（数组 + 链地址法处理冲突），平均 **O(1)** 查找，但**遍历顺序无意义**（跟哈希分布有关，不是插入序也不是 key 序）。

```cpp
#include <unordered_map>
std::unordered_map<std::string, int> cnt;
cnt["apple"]++;                  // 平均 O(1) 查找 + 修改
```

### 3.3 怎么选（面试必问：map vs unordered_map）

| | map / set | unordered_map / unordered_set |
|---|---|---|
| 底层 | 红黑树 | 哈希表 |
| 查找/增删 | O(log n) | 平均 O(1)，最坏 O(n)（大量冲突） |
| 元素顺序 | 按 key 有序 | 无序 |
| 需要 key 的能力 | 可比较（`<`） | 可哈希（`std::hash`）+ 可 `==` |
| 内存 | 较省 | 较费（桶 + 负载因子留空） |

选择原则：
- **需要有序遍历、范围查询（`lower_bound`）、或找最小/最大** → `map`/`set`。
- **只要按 key 快速查/存、不关心顺序**（绝大多数场景，如计数、去重、缓存） → `unordered_map`/`unordered_set`，更快。

---

## 4. `std::pair` 与 `std::tuple`

`pair` 把两个值捆成一个（`map` 的元素就是 `pair<const Key, Value>`）：

```cpp
#include <utility>
std::pair<std::string, int> p{"Tom", 20};
std::cout << p.first << p.second;
auto p2 = std::make_pair("Jerry", 18);   // 自动推类型
auto [name, age] = p2;                    // C++17 结构化绑定拆开
```

`tuple` 是任意个值的捆绑（pair 的推广）：

```cpp
#include <tuple>
std::tuple<int, std::string, double> t{1, "x", 3.14};
std::cout << std::get<1>(t);              // 按下标取："x"
auto [id, name2, val] = t;                // 也能结构化绑定
```

日常用 pair 多，tuple 偶尔用于"临时返回多个值"。

---

## 5. 迭代器（把容器和算法粘起来的胶水）

迭代器是"广义指针"，用法刻意模仿裸指针：`*it` 取值、`++it` 移到下一个、`it != end` 判结束。

```cpp
std::vector<int> v{1, 2, 3};
for (auto it = v.begin(); it != v.end(); ++it)
    std::cout << *it << ' ';
```

- `begin()`：指向第一个元素。
- `end()`：指向**最后一个元素的下一个位置**（哨兵，不可解引用）。这个"左闭右开 `[begin, end)`"区间是 STL 的统一约定。
- `cbegin()/cend()`：const 迭代器，不能通过它改元素。

### 5.1 范围 for 的本质

M1 就用过 `for (int x : v)`，它其实是迭代器循环的语法糖：

```cpp
for (int x : v) { ... }
// 编译器大致展开成：
for (auto it = v.begin(); it != v.end(); ++it) { int x = *it; ... }
```

要**改**元素或**避免拷贝**，用引用：

```cpp
for (auto& x : v) x *= 2;              // 引用，能改，且不拷贝
for (const auto& s : names) use(s);    // 只读大对象，用 const 引用免拷贝
```

### 5.2 迭代器失效（本模块最重要的坑）

迭代器/指针/引用可能因为容器变动而**失效**——继续用它就是未定义行为（可能崩、可能读到垃圾）。

**vector 的失效规则：**
- `push_back`/`insert` 触发**扩容**（size 达到 capacity）时，整块内存被搬到新地址，**所有**迭代器、指针、引用全部失效。
- `erase` 会使**被删位置及其之后**的迭代器失效。

```cpp
std::vector<int> v{1, 2, 3};
int* p = &v[0];              // 指向第一个元素
v.push_back(4);              // 可能扩容 -> p 可能已经悬空！
// std::cout << *p;          // 未定义行为，别这么干

auto it = v.begin();
v.push_back(5);              // 扩容后 it 也失效
// *it;                      // 未定义行为
```

**典型错误：边遍历边 erase**

```cpp
// 错误：erase 后 it 失效，++it 是未定义行为
for (auto it = v.begin(); it != v.end(); ++it)
    if (*it % 2 == 0) v.erase(it);

// 正确：erase 返回"下一个有效迭代器"，用它继续
for (auto it = v.begin(); it != v.end(); )
    if (*it % 2 == 0) it = v.erase(it);   // 删完接住返回值，不 ++
    else              ++it;
```

不同容器失效规则不同：`list` 只有被删的那个节点失效；`map`/`set` 也只有被删元素失效。**vector 最容易踩，因为扩容会全体失效。**

---

## 6. 常用算法 `<algorithm>` / `<numeric>`

算法都接受 `[first, last)` 迭代器区间。挑最常用的：

```cpp
#include <algorithm>
#include <numeric>      // accumulate 在这里，不在 <algorithm>！
#include <vector>

std::vector<int> v{5, 2, 8, 1, 9, 2};

std::sort(v.begin(), v.end());                    // 升序排序
auto it = std::find(v.begin(), v.end(), 8);       // 找值 8，返回迭代器（没找到返回 end()）
int n = std::count(v.begin(), v.end(), 2);        // 统计 2 出现几次
int sum = std::accumulate(v.begin(), v.end(), 0); // 求和，初值 0（注意在 <numeric>）
auto mx = std::max_element(v.begin(), v.end());   // 返回最大元素的迭代器
auto mn = std::min_element(v.begin(), v.end());
```

判断"没找到"的惯用法：拿返回的迭代器和 `end()` 比。

```cpp
if (auto it = std::find(v.begin(), v.end(), 8); it != v.end())
    std::cout << "找到，位置 " << (it - v.begin());
```

**`lower_bound`（在已排序区间二分查找）：** 返回第一个 **>= 目标值** 的位置，O(log n)。前提是区间已有序。

```cpp
std::sort(v.begin(), v.end());
auto lb = std::lower_bound(v.begin(), v.end(), 5);   // 第一个 >=5 的位置
```

带谓词的算法（谓词 = 返回 bool 的可调用对象，下一节的 lambda 就派上用场）：

```cpp
std::find_if(v.begin(), v.end(), pred);    // 找第一个满足 pred 的
std::count_if(v.begin(), v.end(), pred);   // 统计满足 pred 的个数
std::for_each(v.begin(), v.end(), fn);     // 对每个元素执行 fn
std::transform(v.begin(), v.end(), out, fn); // 把每个元素映射后写到 out
```

### 6.1 erase-remove 惯用法（重点，最反直觉）

`std::remove` **并不真的删除**元素——它做不到，算法只有迭代器，够不着容器的 `size`。它把"要保留的元素"往前挪、覆盖掉要删的，返回"新逻辑末尾"的迭代器，后面是垃圾。真正的删除靠容器的 `erase`：

```cpp
std::vector<int> v{1, 2, 3, 2, 4, 2};
// 删掉所有的 2：
v.erase(std::remove(v.begin(), v.end(), 2), v.end());
//       ^^^^^^^^^^ 把非 2 前移，返回新末尾   ^^^^^^ 把新末尾到旧末尾这段真删掉
// v 现在是 {1, 3, 4}
```

按条件删用 `remove_if(begin, end, pred)`，配同样的 `erase`。记死这个"remove_if + erase"组合，工作天天用。

> C++20 起有更省心的 `std::erase`/`std::erase_if(v, ...)`，但你的目标是 C++17，先掌握 erase-remove。

---

## 7. lambda 表达式（现代 C++ 的灵魂）

lambda 是"就地写一个匿名函数"。在 C 里，给 `qsort` 传比较逻辑要在别处单独定义一个函数，割裂又啰嗦；lambda 让你在调用点直接把逻辑写出来。

### 7.1 语法

```cpp
[捕获列表](参数列表) -> 返回类型 { 函数体 }
```

返回类型通常能自动推导，可省略：

```cpp
auto add = [](int a, int b) { return a + b; };
std::cout << add(3, 4);        // 7

// 作为排序的比较器：按降序排
std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });

// 作为谓词：找第一个偶数
auto it = std::find_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
```

### 7.2 捕获列表（lambda 的精髓）

捕获列表 `[]` 决定 lambda 能用到**外部哪些变量、以什么方式**用：

```cpp
int threshold = 5;
int base = 100;

[]              // 什么都不捕获，只能用参数和全局
[threshold]     // 值捕获：拷一份 threshold 进来（此后外部改它，lambda 里不变）
[&threshold]    // 引用捕获：直接用外部那个（外部改了，lambda 里也变）
[=]             // 值捕获所有用到的外部变量
[&]             // 引用捕获所有用到的外部变量
[=, &base]      // 混合：默认值捕获，但 base 用引用
[&, threshold]  // 混合：默认引用捕获，但 threshold 用值
```

```cpp
int threshold = 5;
// 值捕获 threshold，统计大于它的元素个数
int n = std::count_if(v.begin(), v.end(),
                      [threshold](int x) { return x > threshold; });
```

**值捕获 vs 引用捕获的坑：** 引用捕获的是"外部变量本身"，如果 lambda 的生命周期比那个变量长（比如存起来晚点调用、或返回出去），变量已销毁，引用就悬空。**要存起来晚用的 lambda，优先值捕获。**

### 7.3 `mutable`

值捕获的变量在 lambda 内**默认只读**（lambda 的 `operator()` 默认是 const）。想在 lambda 内修改这份拷贝，加 `mutable`：

```cpp
int count = 0;
auto f = [count]() mutable { return ++count; };  // 改的是内部那份拷贝
f(); f();          // 内部 count 变成 2
// 外部 count 仍是 0（值捕获，互不影响）
```

### 7.4 显式返回类型

多数时候能推导。分支返回不同类型、或想强制某类型时，写明 `-> T`：

```cpp
auto f = [](int x) -> double { return x / 2; };   // 强制返回 double
```

> lambda 的本质：编译器帮你生成了一个**带 `operator()` 的匿名类**（叫闭包类型），捕获的变量成了它的成员。所以 lambda 是个对象，能存、能传。这正好接上 M6 的模板和函数对象概念。

---

## 8. `std::function`（统一存放"可调用的东西"）

lambda 每个都是**独立的匿名类型**，没法直接声明一个变量装"任意 lambda"。`std::function<返回类型(参数类型)>` 是个通用容器，能装下签名匹配的**任何**可调用对象：普通函数、lambda、函数对象、绑定结果。

```cpp
#include <functional>

std::function<int(int, int)> op;      // 能装"接收两个 int、返回 int"的任何东西

op = [](int a, int b) { return a + b; };   // 装 lambda
std::cout << op(3, 4);                       // 7

int mul(int a, int b) { return a * b; }
op = mul;                                    // 也能装普通函数
std::cout << op(3, 4);                       // 12
```

最典型的用途是**回调**——把"稍后要执行的动作"作为参数传进去、或存进容器：

```cpp
void onClick(const std::function<void()>& cb) {
    // ... 事件发生时 ...
    cb();
}
onClick([]{ std::cout << "clicked!\n"; });

// 存一堆回调
std::vector<std::function<void()>> handlers;
handlers.push_back([]{ std::cout << "handler A\n"; });
```

> 代价：`std::function` 有类型擦除的开销（可能堆分配、间接调用），比直接用 lambda/模板参数慢。**能用模板参数或 `auto` 传 lambda 就别用 `std::function`**；确实需要"统一类型来存/传异构可调用对象"时才用它。

---

## 9. 组合实战：容器 + 算法 + lambda

这套组合是现代 C++ 的日常写法。感受一下"声明式"风格——你描述"要什么"，而不是手写循环的每一步：

```cpp
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <iostream>

struct Person { std::string name; int age; };

int main() {
    std::vector<Person> people{
        {"Tom", 20}, {"Jerry", 18}, {"Spike", 25}, {"Tyke", 5}
    };

    // 按年龄升序
    std::sort(people.begin(), people.end(),
              [](const Person& a, const Person& b) { return a.age < b.age; });

    // 统计成年人数
    int adults = std::count_if(people.begin(), people.end(),
                               [](const Person& p) { return p.age >= 18; });

    // 所有人年龄之和（accumulate 带自定义累加器）
    int total = std::accumulate(people.begin(), people.end(), 0,
                                [](int acc, const Person& p) { return acc + p.age; });

    std::cout << "成年: " << adults << "，年龄和: " << total << '\n';
}
```

对比 C：这三件事在 C 里是三个手写 for 循环 + 一个单独的 `qsort` 比较函数 + `void*` 强转。STL 版本更短、类型安全、意图清晰。

---

## 10. 常见坑

1. **迭代器失效**：vector `push_back` 扩容后，之前保存的迭代器/指针/引用全失效；`erase` 后被删位置起失效。别缓存迭代器跨越修改操作。
2. **边遍历边 erase**：`erase(it)` 后 `it` 失效，必须用 `it = v.erase(it)` 接住返回值。
3. **`map[key]` 会"顺手插入"**：用 `operator[]` 访问不存在的 key，会**默认构造一个插进去**（value 是 0 / 空串）。只想查、不想插，用 `find` 或 `count`。
   ```cpp
   std::map<std::string, int> m;
   if (m["x"] == 0) { /* 糟糕：这行已经把 "x":0 插进 m 了 */ }
   if (m.find("x") != m.end()) { /* 正确的只读查询 */ }
   ```
4. **`unordered_map` 遍历无序**：别指望它按插入序或 key 序输出。要有序就用 `map`，或倒出来单独 `sort`。
5. **`accumulate` 的初值类型决定结果类型**：`accumulate(v.begin(), v.end(), 0)` 对 `double` 容器会把结果**截成 int**！初值要写 `0.0`。
6. **`std::remove` 不真删**：它只前移 + 返回新末尾，必须配 `erase` 才真的缩短容器（erase-remove 惯用法）。
7. **引用捕获悬空**：lambda 存起来晚用时用 `[&]`，被捕获的局部变量早就销毁了 → 悬空引用。晚用的 lambda 优先值捕获。
8. **`accumulate` 头文件**：它在 `<numeric>`，不是 `<algorithm>`，忘了 include 会报"未声明"。
9. **在范围 for 里增删当前容器**：范围 for 内部缓存了 `end()`，你增删元素它不知道，行为未定义。要改结构用显式迭代器循环。
10. **对未排序区间用 `lower_bound`/`binary_search`**：二分类算法要求区间**已按同一规则有序**，否则结果无意义。

---

## 11. 高频面试点（M7 相关）

- **vector 扩容机制**：`size` vs `capacity`；满了成倍（1.5～2 倍）扩容、搬迁、释放旧内存；为什么成倍——保证 push_back 摊还 O(1)（每次 +1 会退化成 O(n²)）。
- **`reserve` 的作用**：预留 capacity，避免反复扩容搬迁；不改 size。
- **`emplace_back` vs `push_back`**：emplace 就地转发构造，省临时对象；对类类型更优。
- **vector vs list**：连续 vs 链式、随机访问 O(1) vs 不支持、缓存友好度、中间增删代价、迭代器失效范围（见第 2.4 表）。默认用 vector。
- **map vs unordered_map**：红黑树 O(log n) 有序 vs 哈希 O(1) 无序；何时选哪个（见第 3.3 表）。
- **迭代器失效场景**：vector 扩容/erase；边遍历边删的正确写法（`it = erase(it)`）。
- **`map[]` 的副作用**：不存在的 key 会被插入。
- **红黑树 / 哈希表**分别是 map / unordered_map 的底层；哈希最坏 O(n)（冲突）。
- **lambda 的本质**：编译器生成的带 `operator()` 的闭包类；捕获变量成为其成员；值捕获 vs 引用捕获的区别与悬空风险。
- **`std::function` 的代价**：类型擦除、可能堆分配和间接调用，能用模板/auto 就别滥用。
- **erase-remove 惯用法**：为什么 `remove` 不真删、要配 `erase`。

---

## 12. 编译提醒

单文件练习（x64 Native Tools 命令行）：
```
cl /EHsc /std:c++17 /W4 文件名.cpp
```
STL 全是模板，头文件里就是实现，无需额外链接库；照常单文件编译即可。mini 项目也是单文件：
```
cl /EHsc /std:c++17 /W4 word_freq.cpp
```

---

前置回顾：M1 让你初识过 `vector`/`string`，M6 讲了模板机制——STL 正是模板的大规模应用，你现在能看懂 `vector<T>` 为什么对任意 `T` 都成立了。
下一步：打开 `exercises.md`。这一模块的练习会让你把"手写循环"的肌肉记忆，换成"容器 + 算法 + lambda"的现代写法。往后写业务，STL 是你每天都在用的工具。
