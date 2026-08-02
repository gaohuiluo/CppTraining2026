# M1 从 C 到 C++：最小切换

> 目标：把你已有的 C 知识"接上" C++。这一模块不教你写复杂程序，只教你把熟悉的 C 代码改成地道的 C++。全程用「vs C」对比来记。

---

## 0. 一句话总览

C++ 兼容绝大部分 C 语法，所以你已经会一大半了。真正要切换的是这几件事：
**编译模型、输入输出、命名空间、引用、const、以及几个让代码更安全的新关键字。**

---

## 1. 编译与链接模型（和 C 基本一样，但要知道差异）

C++ 沿用 C 的「预处理 → 编译 → 链接」三段式。差异在两点：

### 1.1 名字修饰（name mangling）
C 编译后，函数 `add` 的符号就是 `add`。
C++ 为了支持**函数重载**（同名不同参），会把参数类型编码进符号，`add(int,int)` 可能变成 `?add@@YAHHH@Z`（MSVC）。

这带来一个经典坑：**C++ 调 C 代码要加 `extern "C"`**，否则链接器按 C++ 修饰规则找符号，找不到。

```cpp
extern "C" {
    #include "some_c_lib.h"   // 告诉编译器：这些符号按 C 规则处理
}
```

> 你以后封装 FFmpeg 这类 C 库时天天会用到，先混个眼熟。

### 1.2 头文件写法
C 里包含标准库：`#include <stdio.h>`
C++ 里对应的是：`#include <cstdio>`（去掉 `.h`，前面加 `c`），且函数进入 `std` 命名空间。

| C | C++ |
|---|---|
| `#include <stdio.h>` | `#include <cstdio>` |
| `#include <stdlib.h>` | `#include <cstdlib>` |
| `#include <string.h>` | `#include <cstring>` |
| `#include <math.h>` | `#include <cmath>` |

C++ 独有的头文件不带 `.h`：`<iostream>`、`<string>`、`<vector>`。

---

## 2. 输入输出：`iostream` vs `printf`

```c
// C
#include <stdio.h>
int x = 42;
printf("x = %d\n", x);
```

```cpp
// C++
#include <iostream>
int x = 42;
std::cout << "x = " << x << "\n";
```

对比要点：

- `std::cout` 是「标准输出流」，`<<` 是「插入运算符」，可以链式拼接。
- **不需要格式化占位符**（`%d`/`%s`），编译器根据类型自动选择怎么打印 —— 这就少了一整类 "`%d` 配了个 `double`" 的 bug。
- `std::endl` 会换行**并刷新缓冲区**；`"\n"` 只换行。循环里大量输出用 `"\n"` 更快。
- 输入：`std::cin >> x;`（`>>` 是提取运算符）。

```cpp
int a;
std::cout << "输入一个数: ";
std::cin >> a;
std::cout << "你输入了 " << a << "\n";
```

> 初学先用 `std::cout`。`printf` 在 C++ 里依然能用，但不是地道写法。

---

## 3. 命名空间（namespace）—— C 没有的东西

C 里所有全局名字挤在一个空间，两个库都叫 `init()` 就冲突。C++ 用命名空间隔离。

```cpp
namespace audio {
    void init() { /* ... */ }
}
namespace video {
    void init() { /* ... */ }
}

audio::init();   // :: 是作用域解析运算符
video::init();
```

标准库全部放在 `std` 里，所以是 `std::cout`、`std::string`、`std::vector`。

### 关于 `using namespace std;`
你会在很多教程里看到它，写了之后就能省略 `std::`。**但我建议你现在别用**，原因：

- 它把整个 `std` 倒进当前作用域，容易和你自己的名字撞车。
- 显式写 `std::` 能让你清楚哪些来自标准库 —— 对初学者反而是好事。

真要偷懒，只引入具体的名字：`using std::cout;`。

---

## 4. 引用（reference）—— 重点，C 没有

引用是 C++ 最常用的新概念之一。**它是一个已存在变量的别名**。

```cpp
int a = 10;
int& r = a;    // r 是 a 的别名（注意 & 在类型这边）
r = 20;        // 等价于 a = 20
std::cout << a; // 输出 20
```

### 引用 vs 指针（关键对比表）

| | 指针 `int* p` | 引用 `int& r` |
|---|---|---|
| 可以为空吗 | 可以 `nullptr` | 不能，必须绑定 |
| 可以改指向吗 | 可以，重新赋值指向别处 | 不能，一旦绑定终身不变 |
| 需要解引用吗 | 要 `*p` 才能取值 | 直接用 `r`，像普通变量 |
| 语法负担 | 高 | 低 |

### 最常见的用途：函数参数传引用

```c
// C：想改实参，必须传指针
void swap(int* a, int* b) {
    int t = *a; *a = *b; *b = t;
}
swap(&x, &y);
```

```cpp
// C++：传引用，函数体里像普通变量一样用
void swap(int& a, int& b) {
    int t = a; a = b; b = t;
}
swap(x, y);   // 调用处不用取地址，更干净
```

### const 引用：高效只读传参
传大对象时，值传递会拷贝一份（慢）。用 `const T&` 既不拷贝、又保证不被修改：

```cpp
void print(const std::string& s) {   // 不拷贝 s，且函数内不能改它
    std::cout << s << "\n";
}
```

> 记住这个惯用法：**只读的大对象参数，一律 `const T&`。** 后面模块反复用。

---

## 5. `const` 正确性 —— 比 C 严格得多

C 里 `const` 用得松散。C++ 把 `const` 当成类型系统的一等公民，用途更广、也更被推荐。

```cpp
const int MAX = 100;        // 常量
const int* p = &x;          // p 指向的值不能改（*p 只读）
int* const q = &x;          // q 本身不能改指向（但 *q 能改）
const int* const r = &x;    // 都不能改
```

读法口诀：**从右往左读**。`int* const q` → "q 是 const 的、指向 int 的指针"。

为什么强调它：`const` 能让编译器帮你抓错（你不小心改了不该改的东西，直接编译报错），也让代码意图更清楚。**养成"能 const 就 const"的习惯。**

---

## 6. 几个让代码更安全的新关键字

### 6.1 `nullptr` 取代 `NULL`
C 里 `NULL` 常被定义成 `0`，在重载时会引起歧义。C++ 用 `nullptr`，类型明确。

```cpp
int* p = nullptr;   // 用这个，别用 NULL 或 0
```

### 6.2 `auto` —— 让编译器推断类型
```cpp
auto i = 42;              // int
auto d = 3.14;            // double
auto s = std::string("hi"); // std::string
```
好处是省去写又长又啰嗦的类型（后面 STL 迭代器类型能长到吓人）。**但别滥用**：类型不明显时写出来反而更清楚。

### 6.3 范围 for（range-based for）
```c
// C：手动索引
int arr[5] = {1,2,3,4,5};
for (int i = 0; i < 5; i++) printf("%d ", arr[i]);
```

```cpp
// C++：直接遍历元素
int arr[5] = {1,2,3,4,5};
for (int x : arr) std::cout << x << " ";       // 拷贝每个元素
for (int& x : arr) x *= 2;                      // 引用，可修改
for (const auto& x : arr) std::cout << x;       // 只读、不拷贝（惯用法）
```

---

## 7. `std::string` 和 `std::vector` 初识

这俩会彻底改变你写 C 的方式，M7 会深入，这里先尝个鲜。

### `std::string`：不用再管 `char[]` 和 `\0`
```c
// C：手动管理，容易越界/忘记 \0
char name[20];
strcpy(name, "hello");
strcat(name, " world");
```

```cpp
// C++：自动管理内存，安全
std::string name = "hello";
name += " world";                 // 直接拼接
std::cout << name.size() << "\n";  // 长度
std::cout << name << "\n";
```

### `std::vector`：自动扩容的数组
```c
// C：固定大小，或手动 malloc/realloc
int arr[100];
```

```cpp
// C++：动态数组，自动管理内存
#include <vector>
std::vector<int> v;      // 空
v.push_back(1);          // 追加，自动扩容
v.push_back(2);
std::cout << v.size();   // 2
for (int x : v) std::cout << x << " ";
```

> 结论：**在 C++ 里，几乎不再需要裸 `malloc`/数组来管理动态数据，用 `vector`/`string`。** 这是最大的安全提升之一。

---

## 8. 常见坑（从 C 过来最容易踩的）

1. **`endl` 滥用**：循环里频繁 `std::endl` 会反复刷新缓冲区，慢。用 `"\n"`。
2. **忘了 `std::`**：`cout` 不加 `std::` 且没 `using`，编译报错 "未声明的标识符"。
3. **`using namespace std;` 写在头文件里**：会污染所有包含它的文件，绝对别这么干。
4. **引用必须初始化**：`int& r;` 直接编译错误，引用没有"空"状态。
5. **C++ 里 `char c = 'A';` 的 `'A'` 是 `char`（1 字节）**，而 C 里 `'A'` 是 `int`。多数时候无感，但要知道。
6. **头文件用 `<cstdio>` 后，函数在 `std::` 里**：严格说该写 `std::printf`（实践中 MSVC 通常也接受全局的）。

---

## 9. 高频面试点（M1 相关）

- 引用和指针的区别？（背第 4 节那张表）
- `const int*`、`int* const`、`const int* const` 分别是什么？
- `NULL` 和 `nullptr` 的区别？为什么推荐 `nullptr`？
- `std::endl` 和 `"\n"` 的区别？
- 为什么 C++ 调 C 函数要加 `extern "C"`？（name mangling）

---

## 10. 怎么在 Visual Studio 里编译运行

**方式 A：VS 图形界面**
1. 打开 Visual Studio → 创建新项目 → 选「空项目」（Empty Project）。
2. 右键「源文件」→ 添加 → 新建项 → `.cpp` 文件，把练习代码贴进去。
3. 顶部把配置调成 `x64`，按 `Ctrl+F5`（不调试运行，会停住让你看输出）。
4. 设 C++17：项目右键 → 属性 → C/C++ → 语言 → C++ 语言标准 → 选 `ISO C++17`。

**方式 B：命令行（更快，推荐练手用）**
1. 开始菜单搜 **"x64 Native Tools Command Prompt for VS"**，打开它（这个终端才有 `cl` 环境）。
2. `cd` 到你的 `.cpp` 所在目录。
3. 编译：
   ```
   cl /EHsc /std:c++17 /W4 hello.cpp
   ```
   - `/EHsc`：启用标准异常处理（C++ 常规需要）
   - `/std:c++17`：用 C++17 标准
   - `/W4`：开高等级警告（学习期建议开，能帮你抓问题）
4. 运行：`hello.exe`

> 建议你练手时用**方式 B**，一条命令搞定，改代码重编很快。

---

下一步：打开 `exercises.md`，从练习 1 开始。每题先自己写，卡住了再看 `answers/` 里的参考实现。
