# M7 STL：从内存结构推出一切规则

> 目标:这一篇不给你背结论表。每个容器先画出它在内存里长什么样,然后复杂度、迭代器失效规则、适用场景**全部从图里推导出来**。推导过一遍的东西不用背——你在 C 里手写过链表、动态数组、qsort,这些图对你来说都是老朋友,只是这次它们被封装成了类模板(M6 的成果),内存由 RAII 自动管理(M4 的成果)。

---

## 0. 一句话总览

**STL 用迭代器把「M 种容器 × N 种算法」的 M×N 份代码解耦成 M+N 份;所以学 STL 就是学三件事:容器(数据怎么摆)、迭代器(怎么走)、算法(走的时候干什么),外加 lambda 把「干什么」参数化。**

---

## 1. 主线:STL 到底在解决什么问题

先想一个 C 程序员熟悉的困境。你手写过 `int` 数组的查找、链表的查找,某天需求变成 `struct Person` 的查找——又得写一遍。数一数:

- 数据结构有 M 种:动态数组、链表、哈希表、有序树……
- 操作有 N 种:查找、排序、计数、求和、变换……

朴素做法是每种结构 × 每种操作各写一份,**M×N 份代码**。C 的 `qsort` 尝试用 `void*` + 元素大小 + 函数指针做泛化,代价是类型全丢、到处强转。

STL 的解法分两步:

1. **模板解决「任意类型」**(M6 讲过):`vector<T>` 对任意 `T` 成立,`sort` 对任意元素类型成立,类型不丢。
2. **迭代器解决「任意容器」**:算法不直接认识容器,只认识一对「起点/终点」迭代器。`std::sort` 不知道也不关心你传的是 vector 还是 array——它只要能 `*`(取元素)、`++`(走下一个)、比较(判断到没到头)的东西。

于是 M 种容器各自提供迭代器(M 份),N 种算法只面向迭代器编写(N 份),**M+N 份代码自由组合出 M×N 种能力**。这就是 STL 的全部架构:

```
容器(数据怎么摆) ←── 迭代器(怎么走) ──→ 算法(走的时候干什么)
   vector/map...        begin()/end()        sort/find/count_if...
                                                  ↑
                                        lambda 把「干什么」参数化
```

| 组件 | C 里的对应 | STL 的升级 |
|---|---|---|
| 容器 | 手写数组/链表/哈希表 | 类模板,内存自动管理(RAII) |
| 迭代器 | 裸指针 `int*` 遍历 | 泛化的指针,统一接口 |
| 算法 | 手写 for 循环、`qsort` | 模板函数,类型安全、可内联 |
| 传入的逻辑 | 函数指针 + `void*` ctx | lambda,写在调用点、有类型 |

体验一下组合:

```cpp
#include <vector>
#include <algorithm>
#include <iostream>

int main() {
    std::vector<int> v{5, 2, 8, 1, 9};
    std::sort(v.begin(), v.end());          // 算法 + 容器交出的迭代器
    for (int x : v) std::cout << x << ' ';  // 1 2 5 8 9
}
```

后面整篇的组织就按这三件事来:先讲容器(每个都从内存图推导),再讲迭代器(以及它失效的统一原理),再讲算法和 lambda,最后组合实战。

---

## 2. `std::vector`——连续内存,一切规则的推导起点

vector 是默认容器,也是这篇花最多笔墨的地方,因为**把它的内存模型吃透,后面所有容器都是照着同样的思路推**。

### 2.1 内存结构图

一个 `vector<int>` 本体只有三个指针大小(MSVC x64 上 24 字节),真正的数据在堆上一块**连续**内存里:

```
栈上的 vector 对象                 堆上一块连续内存(int 数组)
+--------+
| begin_ |------------------> +----+----+----+----+----+----+----+----+
| end_   |----------+         | 10 | 20 | 30 | ?  | ?  | ?  | ?  | ?  |
| cap_   |------+   |         +----+----+----+----+----+----+----+----+
+--------+      |   +---------------------^                            ^
                +--------------------------------------------------- --+
   size()     = end_ - begin_ = 3   (有几个元素)
   capacity() = cap_ - begin_ = 8   (这块内存总共能装几个)
```

这就是你在 C 里手写过无数遍的「malloc 数组 + 已用个数 + 总容量」三件套,只是三个字段被封装进了类,析构时自动 `free`(M4 的 RAII)。

从这张图能**直接读出** vector 的第一批性质:

- `v[i]` 就是 `*(begin_ + i)`,一次指针加法,**O(1) 随机访问**——和 C 数组完全一样。
- 元素连续 → 遍历时 CPU 缓存一次载入一整条 cache line(64 字节能装 16 个 int),**缓存极其友好**。这点后面对比 list 时是胜负手。
- 尾部追加:`end_` 处还有空位时,往那儿构造一个元素、`end_++`,**O(1)**。
- 中间插入/删除:连续内存没有「缝」,要在位置 i 插入就得把 i 之后的所有元素**整体后移一格**,删除则整体前移,**O(n)**。

### 2.2 扩容:图上推演一次「搬家」

尾部追加时如果 `end_ == cap_`(满了)怎么办?连续内存**不能原地变长**(后面紧挨着的地址可能已被别人占用),只能搬家:

```
第 1 步:申请一块更大的新内存(MSVC 按 1.5 倍增长,libstdc++ 按 2 倍)
第 2 步:把旧元素逐个搬到新内存(能移动就移动,否则拷贝 —— M5 的知识)
第 3 步:在新内存尾部构造新元素
第 4 步:释放旧内存,三个指针指向新块

搬家前:  begin_ ──> [10|20|30|40]  (size == capacity == 4,满)
                          push_back(50)
搬家后:  begin_ ──> [10|20|30|40|50| ? ]  (新块 capacity 6,旧块已释放)
                     ^^^^^^^^^^^ 这些元素已经不在原来的地址上了!
```

从这次搬家能推导出两条关键规则:

**推论一:扩容后,所有迭代器、指针、引用全部失效。** 不用背——元素都搬到新地址了,旧地址那块内存已经还给系统,你手里存的旧指针指向的是已释放内存,解引用就是未定义行为。

```cpp
std::vector<int> v{1, 2, 3};
int* p = &v[0];          // 指向旧块的第一个元素
auto it = v.begin();     // 同上
v.push_back(4);          // 若触发扩容:元素搬家,旧块释放
// *p、*it 都是未定义行为 —— 反例,别这么写
```

**推论二:搬家搬的是元素,元素是类类型时,搬家成本 = n 次拷贝或移动。** 这里呼应 M5:如果元素类型的移动构造标了 `noexcept`,vector 扩容时会用移动(便宜);没标 `noexcept` 它只敢用拷贝——因为搬到一半抛异常的话,移动过的元素已经被掏空,vector 没法回滚保证强异常安全,而拷贝失败旧数据还完好。**这就是 M5 说「移动构造要标 noexcept」的实际收益点。**

### 2.3 为什么按倍数扩容:算一遍就明白

为什么是 1.5 倍/2 倍,而不是满了就 +1?不用数学证明,拿 2 倍策略从容量 1 开始 push 16 个元素,把「搬家搬了几个元素」数一遍:

```
容量变化:1 → 2 → 4 → 8 → 16
每次扩容要搬的元素个数:  1 + 2 + 4 + 8 = 15 次
push 的元素个数:16 个
总搬运 15 次 < 16 次 push —— 平均每次 push 摊不到 1 次搬运
```

规律:每次扩容搬的量(1+2+4+...+n/2 ≈ n)永远追不上已插入的总量 n,所以 **n 次 push_back 的总成本是 O(n),摊还到每次是 O(1)**。个别那次扩容确实贵(O(n)),但被之后大量便宜的 push 平摊了。

反过来,如果满了只 +1:每次 push 都触发搬家,搬 1+2+3+...+n ≈ n²/2 个元素,**总成本 O(n²)**。面试问「为什么成倍扩容」,答案就是这两笔账。

### 2.4 `reserve`:已知规模,直接跳过所有搬家

既然扩容的成本全在「搬家」,那**提前把 capacity 一次拉够,搬家就一次都不会发生**:

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    for (int i = 0; i < 20; ++i) {
        v.push_back(i);   // 观察 capacity 成倍跳变(MSVC:1,2,3,4,6,9,13,19,28...)
        std::cout << "size=" << v.size() << " cap=" << v.capacity() << '\n';
    }

    std::vector<int> w;
    w.reserve(20);                            // 一次申请到位
    for (int i = 0; i < 20; ++i) w.push_back(i);  // 全程零搬家,cap 恒为 20
    std::cout << "w cap=" << w.capacity() << '\n';
}
```

顺带推论:**reserve 之后、size 到达 capacity 之前,push_back 不会使迭代器失效**(没搬家,元素地址没变)。这也是从图推出来的,不是特例。

两个易混函数,从图上区分:

| | 改哪个指针 | 效果 |
|---|---|---|
| `reserve(n)` | 只挪 `cap_`(换更大的块) | 元素个数不变,只是预留空间 |
| `resize(n)` | 挪 `end_`(必要时也扩块) | 真的变成 n 个元素,多出来的值初始化为 0 |

### 2.5 `push_back` vs `emplace_back`(呼应 M5)

```cpp
std::vector<std::string> v;
v.push_back(std::string("hi"));  // 1. 构造临时 string  2. 移动进容器  3. 销毁临时
v.emplace_back("hi");            // 把 "hi" 转发给 string 构造函数,直接在堆块里原地构造
```

`emplace_back` 少一次移动 + 一次销毁。对 `int` 无差别,对类类型是习惯性优化。这是 M6 完美转发的落地应用。

### 2.6 vs C 手写动态数组

| | C 手写 | `std::vector` |
|---|---|---|
| 结构 | `{T* data; size_t size, cap;}` 自己维护 | 同样三件套,封装在类里 |
| 扩容 | 自己 `realloc`/搬运,忘了就溢出 | 自动,且元素是对象时会正确调用移动/拷贝构造 |
| 释放 | 自己 `free`,漏了就泄漏 | 析构自动释放(RAII) |
| 元素类型 | `void*` 或每种类型写一份 | `vector<T>` 泛型,类型安全 |
| 越界 | 无检查 | `[]` 同样无检查(保持 C 的速度),`at()` 有检查抛异常 |

注意最后一行:`v[i]` **故意不做越界检查**,这是 C++「不为不用的东西付费」的哲学——要检查用 `at(i)`。

### 2.7 `std::array`:栈上定长,vector 的「无堆版」

```
栈上的 std::array<int, 4> 本体就是数据,没有堆分配:
+----+----+----+----+
|  1 |  2 |  3 |  4 |     大小 4 是类型的一部分,编译期确定
+----+----+----+----+
```

```cpp
#include <array>
std::array<int, 4> a{1, 2, 3, 4};
// a.size() == 4,编译期常量;支持迭代器、能整体拷贝、能传给算法
```

它就是 C 原生数组 `int a[4]` 套了层壳:知道自己的 `size()`、不会退化成指针、能直接 `=` 赋值。既然没有堆、没有扩容,自然**永远不存在迭代器失效**(除非对象本身销毁)。大小编译期已知就用它,省一次堆分配。

---

## 3. `std::deque`——分段连续 + 中控数组

vector 头部插入为什么慢?看图:头部前面没有空位,插一个就要全体后移。deque(double-ended queue)为了让**两端都 O(1)**,换了一种内存布局:

```
中控数组(map,存的是指向各段的指针)          若干固定大小的「段」(段内连续)
+------+------+------+------+
|  ?   | seg1 | seg2 |  ?   |         seg1: [ ? | ? | 10 | 20 ]   ← 头部从段中间开始,前面留空
+------+------+------+------+         seg2: [ 30 | 40 | 50 | ?  ] ← 尾部后面也有空
          |      |
          +------+---> 段与段之间地址不连续
```

从这张图推导 deque 的一切:

- **头部 push_front**:seg1 前面还有空位就直接放,O(1);段满了就再挂一个新段到中控数组左边——不需要搬任何已有元素。尾部同理。这就是「两端 O(1)」的来源。
- **随机访问 `d[i]`**:先算 i 落在第几段(一次除法)、段内偏移多少(一次取模),两步定位,仍是 O(1),只是比 vector 的一次加法慢一点。
- **中间插入**:段内还是连续的,还是得搬元素,O(n)。deque 不解决这个问题。
- **迭代器失效规则和 vector 不同,原因在图上**:
  - 两端插入:已有元素**一个都不用搬**(顶多中控数组自己扩一下),所以**指向元素的指针/引用不失效**;但迭代器要维护「当前在哪段」的信息,中控数组变了它就作废——**迭代器失效、引用不失效**,这条独特规则完全由结构决定。
  - 中间插入:搬元素,全失效,同 vector。

```cpp
#include <deque>
#include <iostream>

int main() {
    std::deque<int> d{2, 3};
    d.push_front(1);   // O(1),不搬已有元素
    d.push_back(4);    // O(1)
    for (int x : d) std::cout << x << ' ';   // 1 2 3 4
}
```

用途:需要频繁头部插入/删除(比如队列)时用 deque;否则默认 vector——deque 的分段结构让遍历多一层间接,缓存也不如一整块连续的 vector。

---

## 4. `std::list`——双向链表,和你手写的同构

```
list 对象                堆上散落的节点(每个单独 new)
+------+       +------+------+       +------+------+       +------+------+
| head |<----->| prev | next |<----->| prev | next |<----->| prev | next |
+------+       | 数据: 10    |       | 数据: 20    |       | 数据: 30    |
               +-------------+       +-------------+       +-------------+
                地址 0x7f10...        地址 0x3a28...         地址 0x9c04...  ← 完全不连续
```

这就是你 C 里手写的双向链表,`malloc` 节点、接 `prev/next` 指针那套,一模一样的结构。推导:

- **任意位置插入/删除 O(1)**:改四个指针,不搬任何元素——前提是你**已经拿到那个位置的迭代器**。找到那个位置本身要 O(n) 遍历。
- **没有随机访问**:节点地址毫无规律,`lst[i]` 不存在,想到第 i 个只能从头 `++` 走 i 步。
- **迭代器失效规则最宽松**:插入不动任何已有节点 → 谁都不失效;erase 只 delete 被删的那个节点 → **只有指向被删节点的迭代器失效**,其余全部安好。结构使然,不用背。

### 4.1 为什么实战中 vector 常常反杀 list

理论上「中间插入多就用 list」,实际测起来 vector 经常更快,原因有两个,都在图上:

1. **缓存局部性**。vector 遍历是顺着连续内存走,CPU 预取器闭着眼预取下一条 cache line;list 每走一步都是随机地址跳转,几乎每个节点一次 cache miss。内存访问延迟差百倍量级,搬几个元素的成本经常远小于一路 cache miss 的成本。
2. **先找位置再插入**。list 插入 O(1) 的前提是拿到迭代器,而找位置要 O(n) 次「随机地址跳转」;vector 找位置是 O(n) 次「连续内存扫描」+ O(n) 次连续搬移——两边都是 O(n),但 vector 的每一步都便宜得多。

结论:**默认 vector;只有「拿着迭代器反复在已知位置插删、且元素搬不动(拷贝/移动很贵)」时才考虑 list。**

```cpp
#include <list>
#include <iostream>

int main() {
    std::list<int> lst{1, 2, 3};
    lst.push_front(0);                 // O(1)
    auto it = lst.begin(); ++it;       // 没有 lst[1],只能走过去
    lst.insert(it, 99);                // 在已知位置插入:改指针,O(1)
    for (int x : lst) std::cout << x << ' ';   // 0 99 1 2 3
}
```

### 4.2 序列容器选择小结(全部由结构推出)

| | 内存布局 | 随机访问 | 尾部增删 | 头部增删 | 中间增删 | 缓存 |
|---|---|---|---|---|---|---|
| `vector` | 一整块连续 | O(1) | 摊还 O(1) | O(n) 全搬 | O(n) 搬后半 | 最好 |
| `array` | 栈上连续定长 | O(1) | 不能变长 | 不能 | 不能 | 最好 |
| `deque` | 分段连续+中控 | O(1)(两步) | O(1) | O(1) | O(n) | 中 |
| `list` | 散落节点+指针 | 无 | O(1) | O(1) | O(1)(需迭代器) | 差 |

---

## 5. `std::map` / `std::set`——红黑树:有序是结构自带的

在 C 里「按 key 查值」你要么线性扫,要么手写哈希表,要么维护有序数组 + 二分。C++ 给了两套方案,先看有序的这套。

map/set 底层是**红黑树**——一种自平衡二叉搜索树。你只需要知道两点:它是二叉搜索树(左子树 < 根 < 右子树),且自动保持平衡(树高维持在 O(log n),不会退化成链表)。平衡算法本身不用会。

```
按 key 组织的二叉搜索树(每个节点是堆上单独分配的,含左右孩子指针):

              [ "Jerry" → 18 ]
             /                \
   [ "Amy" → 30 ]        [ "Tom" → 20 ]        ← 左小右大
                                \
                           [ "Zoe" → 25 ]

树高 ≈ log2(n):100 万节点高约 20 层
```

从这棵树推导 map/set 的一切:

- **查找/插入/删除 O(log n)**:从根出发,每层比较一次 key、往左或往右走一步,最多走「树高」步 = O(log n)。100 万条数据约 20 次比较,很快但比哈希慢。
- **遍历天然有序**:对二叉搜索树做中序遍历(左→根→右),出来的顺序就是 key 升序。**有序不是额外功能,是结构白送的。**
- **支持范围查询**:`lower_bound(k)` 沿树走一遍就能找到「第一个 ≥ k」的节点,O(log n)——哈希表做不到这件事(它把 key 打散了)。
- **迭代器稳定**:节点和 list 一样是单独分配的,插入新节点不搬旧节点,erase 只 delete 被删的那个 → **插入不失效任何迭代器,erase 只失效指向被删节点的迭代器**。和 vector 形成鲜明对比,原因还是那句话:元素不搬家。
- **key 需要能比较**:每一步都靠 `<` 决定往左往右,所以 key 类型必须支持 `<`(自定义类型要重载,或传比较器)。

`set` 是只存 key 的树(自动去重),`map` 是节点里再挂个 value 的树:

```cpp
#include <map>
#include <set>
#include <string>
#include <iostream>

int main() {
    std::map<std::string, int> age;
    age["Tom"] = 20;                     // 插入或修改
    age["Jerry"] = 18;
    age["Amy"] = 30;

    for (const auto& [name, a] : age)    // C++17 结构化绑定;中序遍历,天然按 key 字典序
        std::cout << name << ":" << a << '\n';   // Amy Jerry Tom

    std::set<int> s{3, 1, 2, 1};         // 存成 {1,2,3},重复的 1 进不来
    std::cout << s.count(2) << '\n';     // 1
}
```

一个高频坑,先在这儿立牌子:`map[key]` 查不存在的 key 时,**会把这个 key 连同一个值初始化的 value 插进去**(设计如此:`[]` 的语义是「定位,没有就创建」,这样 `age["Tom"] = 20` 才能工作)。只读查询用 `find` 或 `count`:

```cpp
// 反例:这一行已经把 "x"→0 插进 m 了
// if (m["x"] == 0) { ... }
// 正确的只读查询:
// if (m.find("x") != m.end()) { ... }
```

---

## 6. `std::unordered_map` / `std::unordered_set`——桶数组 + 哈希

这就是你 C 里手写的「数组 + 链地址法」哈希表:

```
桶数组(连续)                       每个桶挂一条链(冲突的 key 链在一起)
bucket[0] ──> 空
bucket[1] ──> ["apple"→3] ──> ["grape"→1]     ← 两个 key 哈希撞到同一桶
bucket[2] ──> 空
bucket[3] ──> ["pear"→2]
bucket[4] ──> 空

定位:hash("apple") % 桶数 = 1  →  到 bucket[1] 的链上逐个用 == 比对
```

从图推导:

- **平均 O(1)**:算哈希、取模定位到桶、链上比对几个——只要哈希均匀、桶够多,每条链就只有零星几个元素,常数步搞定。
- **最坏 O(n)**:所有 key 撞进同一个桶时,退化成一条长链,查找变成线性扫。所以「平均 O(1),最坏 O(n)」不是两个孤立结论,是同一张图的两种极端。
- **无序**:元素的位置由 `hash(key) % 桶数` 决定,和插入顺序、key 大小都无关。遍历就是按桶扫,输出顺序无意义。**「无序」不是缺陷,是哈希定位的必然代价。**
- **key 需要 hash + `==` 两样能力**,而且缺一不可:hash 负责跳到桶,`==` 负责在链上确认「就是它」(不同 key 可能同哈希,必须精确比对)。所以自定义类型做 key 要提供 `std::hash` 特化和 `operator==`——map 只要 `<` 就够,这个差异也是结构决定的。
- **rehash 时迭代器全失效**:元素越插越多、链越来越长(负载因子超标)时,容器会分配一个**更大的桶数组**,把所有元素按新桶数**重新取模、重新挂链**——所有元素换了位置,类似 vector 扩容搬家,所以**所有迭代器失效**。(指针/引用不失效:节点本身没重新分配,只是被挂到了别的桶上。)

```cpp
#include <unordered_map>
#include <string>
#include <iostream>

int main() {
    std::unordered_map<std::string, int> cnt;
    cnt["apple"]++;                    // 平均 O(1):定位 + 修改;[] 的「没有就创建」在计数时反而好用
    cnt["pear"]++;
    cnt["apple"]++;
    for (const auto& [w, c] : cnt)     // 顺序无意义,别依赖
        std::cout << w << ":" << c << '\n';
}
```

### 6.1 map vs unordered_map 怎么选(面试必问,答案直接来自两张图)

| | `map`/`set`(树) | `unordered_map`/`unordered_set`(哈希) |
|---|---|---|
| 结构 | 自平衡二叉搜索树 | 桶数组 + 链 |
| 查/增/删 | 稳定 O(log n) | 平均 O(1),最坏 O(n) |
| 遍历顺序 | key 升序(中序遍历白送) | 无意义 |
| 范围查询 `lower_bound` | 支持,O(log n) | 不可能(key 被打散) |
| key 的要求 | 能 `<` | 能 hash + 能 `==` |
| 内存 | 每节点两个孩子指针 | 桶数组要留空(负载因子),偏费 |

选择只有一句话:**需要「有序」或「范围查询」→ map;只要「按 key 快速存取」(计数、去重、缓存,绝大多数场景)→ unordered_map。** 两个理由都指向结构:树天生有序但每步要走 log n 层;哈希一步到位但把顺序打没了。

---

## 7. `std::pair` 与 `std::tuple`——把几个值捆成一个

map 的元素类型就是 `pair<const Key, Value>`(key 是 const 的:它决定元素在树/桶里的位置,改了 key 结构就乱了,所以不许改),遍历 map 时结构化绑定拆的就是这个 pair。

```cpp
#include <utility>
#include <tuple>
#include <string>
#include <iostream>

int main() {
    std::pair<std::string, int> p{"Tom", 20};
    std::cout << p.first << ' ' << p.second << '\n';
    auto [name, age] = p;                       // C++17 结构化绑定拆开

    std::tuple<int, std::string, double> t{1, "x", 3.14};   // pair 的推广:任意个
    std::cout << std::get<1>(t) << '\n';        // 按下标取:"x"
    auto [id, tag, val] = t;                    // 也能结构化绑定
    (void)name; (void)age; (void)id; (void)tag; (void)val;
}
```

日常 pair 用得多(map 元素、函数返回两个值);tuple 偶尔用于临时捆三个以上的值——超过三个建议直接定义 struct,有名字的成员比 `get<2>` 可读。

---

## 8. 迭代器——泛化的指针

### 8.1 本质

裸指针遍历 C 数组的三板斧:`*p` 取值、`++p` 下一个、`p != end` 判结束。迭代器就是**把这三板斧抽象成接口**:任何支持 `*`、`++`、`==`/`!=` 的东西都能当迭代器用。

- vector 的迭代器:本质就是(包装过的)裸指针,`++` 是地址 +sizeof(T)。
- list 的迭代器:包着节点指针的小对象,`++` 是 `p = p->next`。
- map 的迭代器:包着树节点指针,`++` 是「走到中序遍历的下一个节点」。

**算法只依赖这套接口,不关心背后是加法还是指针跳转——这就是第 1 节说的解耦的落地。** 统一约定是左闭右开区间 `[begin, end)`:`begin()` 指向第一个元素,`end()` 指向**最后一个元素的下一个位置**(哨兵,不可解引用)。好处:空容器 `begin() == end()` 自然成立,循环条件统一写 `it != end`。

### 8.2 范围 for 就是迭代器循环的语法糖

```cpp
for (int x : v) { /* ... */ }
// 编译器脱糖成大致这样:
// {
//     auto it  = v.begin();
//     auto ed  = v.end();      // 注意:end 在循环前取好、缓存住
//     for (; it != ed; ++it) { int x = *it; /* ... */ }
// }
```

两个直接推论:

1. `for (int x : v)` 每轮**拷贝**一个元素到 x。要改元素或元素很大,用引用:

```cpp
for (auto& x : v) x *= 2;             // 引用:能改、不拷贝
for (const auto& s : names) use(s);   // 只读大对象:const 引用免拷贝
```

2. **范围 for 里不能增删当前容器**:`end()` 是循环开始前缓存的,你中途增删,缓存的 `ed` 和真实的 end 对不上(vector 扩容的话 it 本身都悬空了),未定义行为。要边遍历边删,用下面 8.4 的显式写法。

### 8.3 迭代器失效:一句话原理 + 汇总

前面每个容器都推过各自的失效规则,现在你会发现它们全是同一句话的特例:

> **元素搬了家、或节点被 delete,指向它的迭代器/指针/引用就失效了;元素没动,就不失效。**

| 容器 | 什么时候元素会「动」 | 结论 |
|---|---|---|
| `vector` | 扩容 → 全体搬新块;erase/中间 insert → 后半段前/后移 | 扩容全失效;erase 后被删位置起失效 |
| `deque` | 两端插入不动旧元素(但中控变) | 两端插入:迭代器失效、指针/引用不失效;中间操作全失效 |
| `list` | 从不搬;erase 只 delete 一个节点 | 只有被删节点的迭代器失效 |
| `map`/`set` | 从不搬;erase 只 delete 一个节点 | 同 list:只失效被删的 |
| `unordered_*` | rehash 时全体重新挂桶 | rehash 迭代器全失效(指针/引用不失效);erase 失效被删的 |

vector 最容易踩,因为「扩容」这个隐形操作会让**全体**失效——你只是 push_back 了一下,老远处存的迭代器就悄悄悬空了。

### 8.4 边遍历边删:标准姿势

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v{1, 2, 3, 4, 5, 6};

    // 反例:erase(it) 之后 it 失效,++it 未定义行为
    // for (auto it = v.begin(); it != v.end(); ++it)
    //     if (*it % 2 == 0) v.erase(it);

    // 正确:erase 返回「被删元素的下一个有效迭代器」,接住它,删的时候不 ++
    for (auto it = v.begin(); it != v.end(); ) {
        if (*it % 2 == 0) it = v.erase(it);
        else              ++it;
    }
    for (int x : v) std::cout << x << ' ';   // 1 3 5
}
```

`map`/`list` 边删同理,也是 `it = m.erase(it)`。

---

## 9. 常用算法——按「你在干什么」分三组

算法都在 `<algorithm>`(累积类在 `<numeric>`),都吃 `[first, last)` 迭代器区间。别按字母背,按用途分三组记。

### 9.1 查找/计数组

```cpp
#include <vector>
#include <algorithm>
#include <iostream>

int main() {
    std::vector<int> v{4, 7, 2, 9, 4, 1, 4};

    // find:找值,返回迭代器;没找到返回 end() —— 判断惯用法就是和 end() 比
    if (auto it = std::find(v.begin(), v.end(), 9); it != v.end())
        std::cout << "找到 9,下标 " << (it - v.begin()) << '\n';

    // find_if:找第一个满足条件的(条件 = 谓词,返回 bool 的可调用对象,lambda 上场)
    auto it2 = std::find_if(v.begin(), v.end(), [](int x) { return x > 5; });
    if (it2 != v.end()) std::cout << "第一个 >5 的是 " << *it2 << '\n';

    std::cout << std::count(v.begin(), v.end(), 4) << '\n';                      // 值计数:3
    std::cout << std::count_if(v.begin(), v.end(),
                               [](int x) { return x % 2 == 0; }) << '\n';        // 条件计数
}
```

**`lower_bound`:已排序区间上二分查找**,返回第一个 ≥ 目标的位置,O(log n)。前提是区间已按同一规则有序——对乱序区间调用,结果是垃圾(二分的每步决策都基于「有序」假设)。

### 9.2 排序组

```cpp
#include <vector>
#include <algorithm>
#include <cstdlib>   // abs
#include <iostream>

int main() {
    std::vector<int> v{-5, 2, -8, 1, 9, -3};

    std::sort(v.begin(), v.end());                                    // 默认按 < 升序
    std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; }); // 传比较器:降序
    std::sort(v.begin(), v.end(),
              [](int a, int b) { return std::abs(a) < std::abs(b); }); // 按绝对值排
    for (int x : v) std::cout << x << ' ';   // 1 2 -3 -5 -8 9
}
```

比较器的语义:`cmp(a, b)` 返回 true 表示「a 应排在 b 前面」。多键排序(先按分数降序、同分按名字升序)也是一个 lambda 写完,后面实战会用到。相等元素要保持原相对顺序时用 `std::stable_sort`。

### 9.3 变换/累积组

```cpp
#include <vector>
#include <algorithm>
#include <numeric>    // accumulate 在这里,不在 <algorithm>!
#include <iostream>

int main() {
    std::vector<int> v{3, 1, 4, 1, 5};

    int sum = std::accumulate(v.begin(), v.end(), 0);        // 求和,初值 0
    // 带自定义累加器:求平方和(acc 是累计值,x 是当前元素)
    int sq  = std::accumulate(v.begin(), v.end(), 0,
                              [](int acc, int x) { return acc + x * x; });

    // transform:逐元素映射,写到目标位置(这里原地写回)
    std::transform(v.begin(), v.end(), v.begin(), [](int x) { return x * 2; });

    auto mx = std::max_element(v.begin(), v.end());   // 返回最大元素的迭代器
    auto mn = std::min_element(v.begin(), v.end());
    std::cout << sum << ' ' << sq << ' ' << *mx << ' ' << *mn << '\n';

    std::for_each(v.begin(), v.end(), [](int x) { std::cout << x << ' '; });
}
```

一个类型陷阱:**accumulate 的结果类型 = 初值的类型**。对 `vector<double>` 写初值 `0`,每一步累加都被截成 int——初值要写 `0.0`。原因看它的模板签名就懂(M6 的报错阅读法):`accumulate(first, last, T init)`,累加变量的类型就是 `T`,你传 `0` 它推出 `T = int`。

### 9.4 erase-remove 惯用法:解耦的代价在这里现形

需求:删掉 vector 里所有的 2。直觉是 `std::remove(v.begin(), v.end(), 2)` 一步到位——但它**不真删**。为什么?回到主线:**算法手里只有迭代器,根本不知道容器是谁,更够不着容器的 size 和内存**。这正是「算法与容器解耦」必须付出的代价:算法能做的只有「通过迭代器读写元素」。

所以 remove 只能干它力所能及的事:把**要保留的元素依次往前挪**,覆盖掉要删的,返回「新逻辑末尾」:

```
remove(begin, end, 2) 前:  [ 1 | 2 | 3 | 2 | 4 | 2 ]
                                 保留的 1,3,4 依次前移覆盖
remove 后:                 [ 1 | 3 | 4 | 2 | 4 | 2 ]
                                         ^ 返回值指这里(新逻辑末尾)
                             |--保留--| |---残留垃圾---|
                             size 没变!后三个是搬移剩下的残值
```

真正把容器截短,得靠**容器自己的成员函数** `erase`(成员函数当然知道容器内部,能挪 `end_`、能析构元素):

```cpp
#include <vector>
#include <algorithm>
#include <iostream>

int main() {
    std::vector<int> v{1, 2, 3, 2, 4, 2};
    v.erase(std::remove(v.begin(), v.end(), 2), v.end());
    //      ^^^^^^^^^^^ 算法:保留者前移,返回新末尾    ^^^ 容器:把 [新末尾, 旧末尾) 真删掉
    for (int x : v) std::cout << x << ' ';   // 1 3 4

    // 按条件删:remove_if + 谓词,同样配 erase
    std::vector<int> w{1, 2, 3, 4, 5, 6};
    w.erase(std::remove_if(w.begin(), w.end(),
                           [](int x) { return x % 2 == 0; }), w.end());
    for (int x : w) std::cout << x << ' ';   // 1 3 5
}
```

「算法逻辑删 + 容器物理删」这对组合记死,工作里天天见。C++20 起有一步到位的 `std::erase(v, 2)` / `std::erase_if(v, pred)`,我们目标 C++17,知道有这回事即可。

---

## 10. lambda——编译器替你写的类

### 10.1 本质:闭包类

lambda 不是什么新物种。你写下:

```cpp
int threshold = 5;
auto f = [threshold](int x) { return x > threshold; };
```

编译器背后生成的大致是这样一个**匿名类**(叫闭包类型),然后 `f` 就是它的一个对象:

```cpp
class __Lambda_7A2F {                 // 名字是编译器编的,你写不出来
    int threshold;                    // 捕获的变量 → 成员变量
public:
    __Lambda_7A2F(int t) : threshold(t) {}
    bool operator()(int x) const {    // 函数体 → operator();注意默认是 const
        return x > threshold;
    }
};
auto f = __Lambda_7A2F{threshold};    // 「捕获」就是用当前值构造这个对象
```

**记住这一个模型,lambda 的所有规则都是推论,不用背:**

| 规则 | 从模型推导 |
|---|---|
| 值捕获后,外部再改变量,lambda 里不变 | 捕获发生在**构造对象那一刻**,拷了一份进成员;之后各改各的 |
| 值捕获的变量在 lambda 里默认改不了 | `operator()` 默认是 **const** 成员函数,const 函数里不能改成员(M2 的规则) |
| 加 `mutable` 就能改,但改的是副本 | `mutable` 去掉 operator() 的 const;改的当然是成员(那份拷贝),外部原变量无关 |
| 引用捕获,外部改了 lambda 里跟着变 | 成员是**引用**,和外部是同一个变量 |
| 引用捕获有悬垂风险 | 成员引用指向外部变量;lambda 对象活得比那个变量久(存起来晚调用/返回出去),引用就悬空——和 M4 的悬垂指针同款问题 |
| 无捕获 lambda 能转成函数指针 | 没有捕获 → 类没有成员 → operator() 不依赖对象状态,等价于普通函数 |
| 每个 lambda 类型都不同、写不出来 | 每写一个 lambda 就生成一个新的匿名类 |

逐条看代码:

```cpp
#include <iostream>

int main() {
    // 语法全貌:[捕获列表](参数) -> 返回类型 { 函数体 },返回类型通常可推导省略
    auto add = [](int a, int b) { return a + b; };
    std::cout << add(3, 4) << '\n';                    // 7

    // 值捕获:构造时拷贝
    int t = 5;
    auto ge = [t](int x) { return x >= t; };
    t = 100;                                           // 外部改了
    std::cout << ge(10) << '\n';                       // 仍按 t=5 判断:1

    // mutable:去掉 operator() 的 const,可改副本
    int count = 0;
    auto tick = [count]() mutable { return ++count; }; // 改的是成员那份
    tick(); tick();
    std::cout << count << '\n';                        // 外部 count 仍是 0

    // 引用捕获:成员是引用,直接操作外部变量
    int sum = 0;
    auto acc = [&sum](int x) { sum += x; };
    acc(3); acc(4);
    std::cout << sum << '\n';                          // 7

    // 无捕获 → 可转函数指针(没有成员,不需要对象状态)
    int (*fp)(int, int) = [](int a, int b) { return a * b; };
    std::cout << fp(3, 4) << '\n';                     // 12

    // 显式返回类型:分支返回不同类型或想强制时写 -> T
    auto half = [](int x) -> double { return x / 2.0; };
    std::cout << half(5) << '\n';                      // 2.5
}
```

捕获列表的完整写法:

```cpp
[]              // 不捕获:只能用参数和全局量
[t]             // 值捕获 t
[&t]            // 引用捕获 t
[=]             // 用到的外部变量全部值捕获
[&]             // 全部引用捕获
[=, &sum]       // 默认值捕获,sum 例外用引用
[&, t]          // 默认引用捕获,t 例外用值
```

经验法则(由悬垂推论直接得出):**当场用完的 lambda,`[&]` 方便没问题;要存起来晚点调用的 lambda,优先值捕获**——它把依赖的数据都拷成了自己的成员,自带干粮,不怕外部变量先死。

### 10.2 vs C 的回调模式:qsort 对 sort

C 里把「逻辑」传给函数只有一条路:函数指针;要携带上下文,还得再传个 `void* ctx`。对比同一个需求「按绝对值排序」:

```cpp
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <iostream>

// C 版:比较函数必须单独定义在外面,void* 强转,类型安全全无
int cmp_abs(const void* a, const void* b) {
    int x = std::abs(*static_cast<const int*>(a));
    int y = std::abs(*static_cast<const int*>(b));
    return (x > y) - (x < y);
}

int main() {
    int arr[] = {-5, 2, -8, 1};
    std::qsort(arr, 4, sizeof(int), cmp_abs);   // 指针、个数、大小、函数指针,四件套

    std::vector<int> v{-5, 2, -8, 1};
    std::sort(v.begin(), v.end(),               // C++ 版:逻辑写在调用点,有类型
              [](int a, int b) { return std::abs(a) < std::abs(b); });
    for (int x : v) std::cout << x << ' ';      // 1 2 -5 -8
}
```

差距不止可读性。`qsort` 每比较一次都要**通过函数指针间接调用** `cmp_abs`,编译器无法内联;`std::sort` 是模板,lambda 的闭包类型是模板实参的一部分,`operator()` 直接内联进排序代码——**sort 通常比 qsort 快**,「更抽象反而更快」,因为泛型在编译期展开(M6 讲过模板实例化)。另外,lambda 的捕获替代了 `void* ctx`:上下文成了闭包的成员,类型安全地带进去,不用强转。

---

## 11. `std::function`——给「各不相同的可调用类型」一个统一的壳

上一节推论说:每个 lambda 都是独立的匿名类型,而且名字写不出来。当场传给模板算法没问题(模板会推导),但两个场景卡壳了:

1. **存**:`std::vector<??> callbacks;` ——问号处写什么?每个 lambda 类型都不同,写不出统一的元素类型。
2. **接口边界**:头文件里声明 `void onClick(?? cb);`,普通函数(非模板)必须写死参数类型。

`std::function<签名>` 就是这个统一的壳:**只要签名匹配(参数、返回值能对上),任何可调用的东西——普通函数、lambda(带不带捕获都行)、函数对象——都能装进去**。

```cpp
#include <functional>
#include <vector>
#include <iostream>

int negate(int x) { return -x; }

void repeat(int n, const std::function<void(int)>& action) {   // 回调作参数
    for (int i = 0; i < n; ++i) action(i);
}

int main() {
    std::function<int(int)> op;              // 能装「收一个 int 返回 int」的任何东西
    op = [](int x) { return x * x; };        // 装 lambda
    std::cout << op(5) << '\n';              // 25
    op = negate;                             // 换装普通函数
    std::cout << op(5) << '\n';              // -5

    repeat(3, [](int i) { std::cout << "第" << i << "次\n"; });

    std::vector<std::function<void()>> tasks;          // 存一堆异构回调
    int base = 10;
    tasks.push_back([] { std::cout << "task A\n"; });
    tasks.push_back([base] { std::cout << "task B, base=" << base << "\n"; });
    for (const auto& t : tasks) t();
}
```

它怎么做到的?直觉版的**类型擦除**:`std::function` 内部把你塞进来的对象拷到一块自己管理的存储里(对象大了就上堆),再通过一层统一的间接调用接口去调它——具体类型被「擦掉」,对外只剩签名。够用即止,不展开。

代价从原理直接推出:**可能一次堆分配(存闭包)+ 每次调用一层间接跳转(没法内联)**。所以准则是:能用模板参数或 `auto` 直接传 lambda 就别用 `std::function`;只有确实需要「统一类型」——存进容器、跨接口边界传回调——才用它。

---

## 12. 组合实战:容器 + 算法 + lambda

三件事合流。感受声明式风格——描述「要什么」,而不是手写循环的每一步:

```cpp
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <iostream>

struct Student { std::string name; int score; };

int main() {
    std::vector<Student> ss{
        {"Tom", 72}, {"Jerry", 55}, {"Spike", 90}, {"Tyke", 88}
    };

    // 多键排序:分数降序,同分按名字字典序升序 —— 一个 lambda 写完
    std::sort(ss.begin(), ss.end(), [](const Student& a, const Student& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.name < b.name;
    });

    // 及格人数
    int pass = static_cast<int>(std::count_if(ss.begin(), ss.end(),
                   [](const Student& s) { return s.score >= 60; }));

    // 平均分:accumulate 自定义累加器;初值 0.0 保证按 double 算
    double avg = std::accumulate(ss.begin(), ss.end(), 0.0,
                     [](double acc, const Student& s) { return acc + s.score; })
                 / static_cast<double>(ss.size());

    // 第一个不及格的
    auto it = std::find_if(ss.begin(), ss.end(),
                  [](const Student& s) { return s.score < 60; });

    // 只提取名字:transform 到新容器(back_inserter 边插边扩,不用预先 resize)
    std::vector<std::string> names;
    std::transform(ss.begin(), ss.end(), std::back_inserter(names),
                   [](const Student& s) { return s.name; });

    std::cout << "及格 " << pass << " 人,平均 " << avg << '\n';
    if (it != ss.end()) std::cout << "第一个不及格: " << it->name << '\n';
    for (const auto& n : names) std::cout << n << ' ';
}
```

对比 C:这是四个手写 for 循环 + 一个单独定义的 qsort 比较函数 + 一堆 `void*`。STL 版每一行都直说意图。

mini 项目「词频统计」就是这套组合的完整版:`unordered_map<string,int>` 计数(哈希 O(1),第 6 节)→ 倒进 `vector<pair<string,int>>`(第 7 节)→ `sort` + 多键 lambda 按频率排(本节套路)→ 输出 Top N。做到那题时回来翻这几节。

---

## 13. 常见坑

1. **push_back 后继续用旧迭代器/指针**:扩容 = 搬家 + 释放旧块,全体失效(2.2 的推论)。跨越修改操作就别缓存迭代器;实在要缓存,先 `reserve` 够。
2. **边遍历边 erase 不接返回值**:`erase(it)` 后 `it` 指向已析构位置,必须 `it = v.erase(it)` 接住(8.4)。
3. **`map[key]` 顺手插入**:`[]` 的语义是「定位,没有就创建」。只读查询用 `find`/`count`(第 5 节)。
4. **指望 unordered_map 有序输出**:位置由 `hash % 桶数` 决定,顺序无意义。要有序用 map,或倒出来 sort(第 6 节)。
5. **`accumulate` 初值类型截断**:结果类型 = 初值类型,double 求和初值要写 `0.0`(9.3)。
6. **`remove` 后不 erase**:remove 只有迭代器、够不着 size,只能前移覆盖;必须配容器的 erase 收尾(9.4)。
7. **引用捕获的 lambda 存起来晚用**:成员引用指向的局部变量早已销毁,悬垂。晚用的 lambda 值捕获,自带干粮(10.1)。
8. **`accumulate` 忘了 `#include <numeric>`**:它不在 `<algorithm>`,报「未声明的标识符」时先查这个。
9. **范围 for 里增删当前容器**:end 在循环前缓存,结构一变就对不上,未定义行为(8.2)。
10. **对乱序区间用 `lower_bound`/`binary_search`**:二分的每一步都建立在「有序」假设上,乱序区间上结果是垃圾(9.1)。
11. **STL 报错刷屏就慌**:容器算法全是模板,报错是 M6 说的「模板错误瀑布」——只看第一条 error、找到自己源文件的行号,通常就是类型对不上或少了运算符(比如自定义类型没提供 `<` 就丢进 set)。

---

## 14. 高频面试点(附答案要点)

- **vector 扩容机制?** 三指针模型(begin/end/cap);满了申请更大块(1.5~2 倍)→ 搬元素(优先 noexcept 移动,否则拷贝)→ 释放旧块。成倍是为了摊还 O(1):搬运总量 1+2+4+...≈n,每次 +1 则总量 ≈n²/2。
- **reserve 和 resize 区别?** reserve 只提 capacity 不改 size,用于已知规模避免反复搬家;resize 真改元素个数(新元素值初始化)。
- **emplace_back vs push_back?** emplace 把参数完美转发进容器内存原地构造,省一次临时对象的移动 + 析构;类类型有收益,int 无差别。
- **vector vs list?** 连续 vs 节点散落 → 随机访问 O(1) vs 无、缓存友好 vs 每步 cache miss、中间插删 O(n) 搬移 vs O(1) 改指针(但找位置仍 O(n))。实战默认 vector,缓存局部性经常让它在「理论上该 list 赢」的场景也更快。
- **map vs unordered_map?** 红黑树 O(log n)、中序遍历有序、支持范围查询、key 要 `<`;哈希表平均 O(1) 最坏 O(n)(全撞一桶)、无序、key 要 hash+`==`。要有序/范围查 → map,纯键值存取 → unordered_map。
- **迭代器失效的原理?** 一句话:元素搬家或节点被删,指向它的就失效。vector 扩容全失效;list/map 只失效被删节点;unordered rehash 全失效。边删边遍历用 `it = c.erase(it)`。
- **`map[]` 的副作用?** 不存在的 key 会被值初始化后插入;只读查询用 find/count。
- **erase-remove 为什么两步?** 算法只持迭代器,不知道容器、改不了 size(解耦的代价),remove 只能前移覆盖返回新逻辑末尾;erase 是成员函数才能真正截短。C++20 有 `std::erase_if` 一步到位。
- **lambda 的本质?** 编译器生成的匿名类(闭包类型):捕获 → 成员,函数体 → `operator()`(默认 const)。由此推出:值捕获是构造时拷贝、mutable 才能改副本、引用捕获有悬垂风险、无捕获可转函数指针、每个 lambda 类型唯一。
- **std::function 的代价?** 类型擦除:可能堆分配存闭包 + 间接调用无法内联。能用模板/auto 传就不用它;需要统一类型存储或跨接口传回调才用。
- **sort 为什么常比 qsort 快?** sort 是模板,比较器类型编译期已知,能内联;qsort 每次比较走函数指针间接调用,还要 void* 强转。

---

## 15. 编译提醒

单文件练习(x64 Native Tools 命令行):

```
cl /EHsc /std:c++17 /W4 文件名.cpp
```

STL 全是模板,头文件即实现,无需额外链接库。mini 项目同样单文件:

```
cl /EHsc /std:c++17 /W4 word_freq.cpp
```

---

## 16. 承前启后

回头看,这一模块几乎每个知识点都踩在前面模块的肩膀上:

- **M4(RAII)**:容器自动管理内部内存——vector 析构时释放堆块、list/map 析构时逐个 delete 节点。你全程没写一个 `new`/`delete`,这就是 RAII 的胜利。
- **M5(拷贝/移动)**:vector 扩容搬家用移动还是拷贝、`noexcept` 为什么关键、emplace_back 省的是什么,答案全在 M5。
- **M6(模板)**:容器是类模板、算法是函数模板、lambda 传给算法靠模板参数推导;STL 报错瀑布用 M6 的「只看第一条」阅读法。

往后的伏笔:

- **M8** 会讲 `string_view`(只读字符串参数不再拷贝)和更多 C++17 便利设施——你已经在用的结构化绑定 `for (const auto& [k, v] : m)` 就是其中之一。
- **Capstone(音视频)**:WAV 文件解析会用 `vector<uint8_t>` 装整段字节流——`reserve` 预留文件大小、连续内存直接按偏移解析头部,这一模块的 vector 功夫会直接变现。

下一步:打开 `exercises.md`,把「手写循环」的肌肉记忆换成「容器 + 算法 + lambda」。练到 mini 项目词频统计时,你会发现整篇文章在一个 60 行的程序里全部出现了一遍。
