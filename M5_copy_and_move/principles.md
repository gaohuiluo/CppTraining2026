# M5 拷贝、移动与 Rule of 0/3/5（完整）

> 目标：把「一个对象被复制、被搬走时到底发生了什么」彻底讲透。这是 C 里**完全没有**的概念——C 里 `struct s2 = s1;` 就是逐字节 `memcpy`，仅此而已；而 C++ 把「复制」和「搬移」变成了可以由你定义、由编译器自动调用的**函数**。管理堆内存的类如果不管这件事，轻则内存泄漏，重则 double free 崩溃。这一模块讲全：拷贝构造/拷贝赋值、浅拷贝 vs 深拷贝、右值引用、移动语义、Rule of 0/3/5、`=default`/`=delete`、RVO 与 noexcept。

---

## 0. 一句话总览

**一个对象「诞生成另一个对象的副本」叫拷贝，「把另一个对象的家当直接抢过来」叫移动。**
当你的类持有堆资源（`new` 出来的指针）时，编译器默认帮你做的拷贝是**危险的浅拷贝**，你必须自己接管；而移动能避免昂贵的深拷贝，是现代 C++ 性能的关键。理解 Buffer 这个例子（持有 `int* data_`），全模块就通了。

---

## 1. 从 C 说起：`=` 在 C 里只是 memcpy

C 里结构体赋值，编译器就给你逐字节拷贝：

```c
typedef struct { int* data; int size; } Buffer;

Buffer a = make_buffer(100);   // a.data 指向堆上 100 个 int
Buffer b = a;                  // 逐字节拷贝：b.data == a.data ！！两个指针指同一块内存
free(a.data);                  // 释放了
free(b.data);                  // 再释放一次 —— double free，崩溃
```

C 里这个问题**只能靠程序员自觉**：你得记得「这个 struct 里有指针，不能直接 `=`，要手写一个 `buffer_copy()` 深拷贝函数」。没有任何机制强制你、也没有机制在赋值时自动帮你。

C++ 的答案是：**赋值和初始化都是可重载的函数**。你可以定义「当我这个类型被拷贝时，具体该怎么做」，编译器会在该拷贝的地方自动调用它。

| | C | C++ |
|---|---|---|
| `b = a`（含指针成员） | 永远逐字节拷贝，指针共享 | 调用你定义的拷贝赋值，可做深拷贝 |
| 传参 / 返回结构体 | 逐字节拷贝 | 调用拷贝/移动构造 |
| 谁保证正确 | 程序员自觉 | 类型自己负责（RAII 思想） |

---

## 2. 四个「特殊成员函数」先认脸

围绕拷贝和移动，一共有四个特殊成员函数（加上析构，就是后面 Rule of 5 的五个）。先记住它们的签名：

```cpp
class Buffer {
public:
    Buffer(const Buffer& other);              // 1. 拷贝构造：用一个已存在的对象造出新对象
    Buffer& operator=(const Buffer& other);   // 2. 拷贝赋值：把已存在对象的内容赋给另一个已存在对象
    Buffer(Buffer&& other) noexcept;          // 3. 移动构造：把 other 的家当搬进新对象
    Buffer& operator=(Buffer&& other) noexcept;// 4. 移动赋值：把 other 的家当搬给已存在对象
    ~Buffer();                                 // 5. 析构（M2 学过）
};
```

- **构造 vs 赋值**的区别：构造是「无中生有造一个新对象」，赋值是「一个已经活着的对象接收新内容」。所以拷贝/移动各有构造和赋值两个版本。
- **拷贝 vs 移动**的区别：拷贝要保证源对象不变（参数是 `const&`），移动允许掏空源对象（参数是 `&&`，非 const）。

区分构造和赋值，看等号左边是不是**正在诞生**：

```cpp
Buffer a(100);
Buffer b = a;    // b 正在诞生 -> 拷贝构造（这里的 = 是初始化，不是赋值！）
Buffer c;
c = a;           // c 早就活着了 -> 拷贝赋值 operator=
```

> 这是从 C 过来最容易懵的点：`Buffer b = a;` 里的 `=` **不是赋值**，是初始化，调的是**拷贝构造**。只有对一个已存在对象写 `=` 才是拷贝赋值。

---

## 3. 拷贝构造函数：什么时候被调用

拷贝构造函数 `Buffer(const Buffer& other)` 在「需要用一个已有对象产生一个新副本」时被调用。四种典型场景：

```cpp
void byValue(Buffer b);          // 参数按值传递
Buffer makeOne();                // 按值返回

Buffer a(100);
Buffer b = a;      // (1) 拷贝初始化：用 a 造 b
Buffer c(a);       // (2) 直接初始化：同样是拷贝构造
byValue(a);        // (3) 传参：形参 b 是 a 的副本 -> 拷贝构造
Buffer d = makeOne(); // (4) 返回：理论上要拷贝，但通常被 RVO 优化掉（见第 10 节）
```

要点：
- 参数**必须**是引用 `const Buffer&`。如果写成 `Buffer(Buffer other)`（按值），那传参本身又要拷贝一次，无限递归——编译器直接不让你这么写。
- 用 `const` 是因为拷贝不应改动源对象。

> vs C：C 里传结构体、返回结构体都是编译器 memcpy，你无从干预。C++ 把这四个时机都变成了「调用拷贝构造」，于是你能插手，做正确的深拷贝。

---

## 4. 浅拷贝 vs 深拷贝（全模块的核心）

这是整个模块最关键的一节。用一个持有堆内存的 `Buffer` 演示。

### 4.1 编译器默认生成的拷贝 = 浅拷贝 = 灾难

你要是**什么都不写**，编译器会自动生成一个拷贝构造和拷贝赋值，行为是**逐成员拷贝**（对指针成员就是拷贝指针的值，即地址）：

```cpp
class Buffer {
public:
    Buffer(int n) : size_(n), data_(new int[n]) {}   // 构造：堆上分配 n 个 int
    ~Buffer() { delete[] data_; }                    // 析构：释放
    // 没写拷贝构造/拷贝赋值 -> 编译器生成逐成员拷贝的版本
private:
    int  size_;
    int* data_;
};

void bad() {
    Buffer a(100);
    Buffer b = a;    // 编译器生成的浅拷贝：b.data_ = a.data_ (只拷了地址!)
}                    // 作用域结束：先 ~b 释放那块内存，再 ~a 又释放同一块 -> double free 崩溃
```

问题拆开看：
- 浅拷贝后 `a.data_` 和 `b.data_` **指向同一块堆内存**（和 C 的 memcpy 一模一样）。
- 两个对象析构时各 `delete[]` 一次 → 同一块内存被释放两次 → **double free**。
- 就算没 double free，改 `b` 的数据会莫名其妙影响 `a`（**别名/共享**）；如果一个先析构了，另一个就变成**悬垂指针**，再用就是 use-after-free。

这正是第 1 节 C 语言那个坑，只不过 C++ 里它藏在「一句不起眼的 `Buffer b = a;`」背后，更隐蔽。

### 4.2 深拷贝：自己开一块，把内容复制过去

正确做法是让副本拥有**自己的**那块内存：

```cpp
class Buffer {
public:
    Buffer(int n) : size_(n), data_(new int[n]) {}
    ~Buffer() { delete[] data_; }

    // 深拷贝构造：分配自己的内存，再逐元素复制内容
    Buffer(const Buffer& other) : size_(other.size_), data_(new int[other.size_]) {
        std::copy(other.data_, other.data_ + size_, data_);
    }

    // 深拷贝赋值：见下一节（有更多讲究）
    Buffer& operator=(const Buffer& other);
private:
    int  size_;
    int* data_;
};
```

现在 `Buffer b = a;` 之后，`b.data_` 是一块全新的内存，内容和 `a` 相同但互不干扰，各自析构各释放各的，安全。

| | 浅拷贝（默认生成） | 深拷贝（手写） |
|---|---|---|
| 指针成员 | 拷贝地址，两对象共享同一块内存 | 新分配一块，复制内容 |
| 析构 | double free / 悬垂 | 各管各的，安全 |
| 改一个 | 影响另一个 | 互不影响 |
| 开销 | 小（只拷指针） | 大（要分配 + 复制） |

> 结论：**只要类里有裸指针（或任何独占的资源句柄），编译器默认的浅拷贝几乎一定是错的，你必须自己写深拷贝。** 这直接引出后面的 Rule of 3。

---

## 5. 拷贝赋值运算符：比拷贝构造多三件事

拷贝赋值 `operator=` 处理的是「两个都已经活着的对象」：`c = a;`。它比拷贝构造麻烦，因为左边的 `c` **已经持有一块旧内存**，你得先处理掉它。

一个正确的拷贝赋值要做到：
1. **防自赋值**：`a = a;` 时别把自己释放了再拷贝自己。
2. **释放旧资源**：`c` 原来那块内存要 `delete[]`，否则泄漏。
3. **分配新资源、复制内容**。
4. **返回 `*this`**：支持链式 `a = b = c`（回忆 M2 的 `return *this`）。

```cpp
Buffer& Buffer::operator=(const Buffer& other) {
    if (this == &other) return *this;        // 1. 自赋值检查
    int* newData = new int[other.size_];     //    先分配新的(万一 new 抛异常，*this 还完好)
    std::copy(other.data_, other.data_ + other.size_, newData);
    delete[] data_;                          // 2. 释放旧资源
    data_ = newData;                         // 3. 接管新资源
    size_ = other.size_;
    return *this;                            // 4. 支持链式赋值
}
```

注意顺序：**先分配好新的，再删旧的**。如果先 `delete[] data_` 再 `new`，一旦 `new` 抛异常，`data_` 就成了悬垂指针，对象被破坏。这叫「强异常安全」的雏形。

> 进阶写法有 **copy-and-swap**（用拷贝构造 + `std::swap` 一次搞定自赋值和异常安全），练习里会让你见识；入门先把上面这个「检查-分配-释放-接管」四步走记牢。



#### 深拷贝赋值：Copy-and-Swap 

##### 5.1. 完整代码示例

```cpp
#include <iostream>
#include <algorithm> // std::copy
#include <utility>  // std::swap
#include <string>

class Buffer {
private:
    int* data_;
    size_t size_;
    std::string tag_; // 加个标签方便打印看是谁

public:
    // 1. 普通构造函数
    Buffer(size_t size, std::string tag) : data_(new int[size]), size_(size), tag_(tag) {
        std::cout << "  [构造] " << tag_ << " 产生\n";
    }

    // 2. 拷贝构造函数 (深拷贝)
    Buffer(const Buffer& other) : data_(new int[other.size_]), size_(other.size_), tag_("拷贝自(" + other.tag_ + ")") {
        std::copy(other.data_, other.data_ + size_, data_);
        std::cout << "  [拷贝构造] 造了一个 " << tag_ << "\n";
    }

    // 3. 析构函数
    ~Buffer() {
        std::cout << "  [析构] 销毁 " << tag_ << " (释放内存)\n";
        delete[] data_;
    }

    // 4. 友元 swap 函数 (只交换指针，极快且不抛异常 noexcept)
    friend void swap(Buffer& a, Buffer& b) noexcept {
        using std::swap;
        swap(a.data_, b.data_);
        swap(a.size_, b.size_);
        swap(a.tag_, b.tag_);
    }

    // ==========================================
    // 5. 进阶版赋值运算符 (Copy-and-Swap)
    // ==========================================
    Buffer& operator=(Buffer other) noexcept { 
        // 注意：参数 other 是按值传递的，这里它已经是一个拷贝好的副本了！
        std::cout << "  [operator=] 开始交换 *this 和 " << other.tag_ << "\n";
        
        swap(*this, other); // 把当前对象和副本交换
        
        std::cout << "  [operator=] 交换完毕，当前对象现在是 " << tag_ << "\n";
        return *this;
    } // 离开作用域，other 析构，带走旧内存！

    void printInfo() const {
        std::cout << "我是 " << tag_ << "，大小为 " << size_ << "\n";
    }
};

int main() {
    std::cout << "--- 创建 buf1 和 buf2 ---\n";
    Buffer buf1(10, "buf1");
    Buffer buf2(20, "buf2");

    std::cout << "\n--- 开始执行 buf1 = buf2 ---\n";
    buf1 = buf2; 

    std::cout << "\n--- 赋值结束后的状态 ---\n";
    buf1.printInfo();
    buf2.printInfo();

    return 0;
}
```

---

##### 5.2. 运行结果大揭秘

运行这段代码，你会清楚地看到“按值传递”和“自动析构”是怎么配合工作的：

```text
--- 创建 buf1 和 buf2 ---
  [构造] buf1 产生
  [构造] buf2 产生

--- 开始执行 buf1 = buf2 ---
  [拷贝构造] 造了一个 拷贝自(buf2)        <-- 1. 发生在传参时（按值传递）
  [operator=] 开始交换 *this 和 拷贝自(buf2) <-- 2. 交换内部指针
  [operator=] 交换完毕，当前对象现在是 拷贝自(buf2)
  [析构] 销毁 buf1 (释放内存)              <-- 3. 临时副本析构，带走了 buf1 原来的旧内存！

--- 赋值结束后的状态 ---
我是 拷贝自(buf2)，大小为 20
我是 buf2，大小为 20

  [析构] 销毁 buf2 (释放内存)
  [析构] 销毁 拷贝自(buf2) (释放内存)
```

看到了吗？因为按值传递，编译器偷偷帮你调用了**拷贝构造函数**造了一个副本。函数结束时，这个副本又偷偷帮你调用了**析构函数**清理了旧内存！

---

##### 5.3. 把传递和返回的类型掰开揉碎讲

我们重点看这一行：
```cpp
Buffer& operator=(Buffer other) noexcept {
    ...
    return *this; 
}
```

##### 问题 1：参数传递的类型为什么是 `Buffer other`（按值传递）？

传统的写法是 `const Buffer& other`（按const引用传递），但 Copy-and-Swap **故意不用引用，而是按值传递**。这是这种写法最绝妙的设计：

* **按值传递会触发“拷贝构造”**：当执行 `buf1 = buf2` 时，编译器会先用 `buf2` 拷贝构造出一个**全新的、临时的局部对象** `other`。这就是“Copy”的阶段。
* **天然异常安全**：如果内存不足，拷贝构造失败了（抛出异常），此时还在函数外边，`buf1`（也就是 `*this`）根本没有被修改过，毫发无伤。
* **天然防自赋值**：如果是 `buf1 = buf1` 呢？编译器会拷贝出一个 `buf1` 的副本 `other`，然后把 `buf1` 和 `other` 互换。逻辑完全正确，绝对不会把自己的内存删了再去读。
* **充当临时缓冲区**：这个 `other` 拥有独立分配好的新内存。我们只需要把它和当前对象的数据“指针一换”，当前对象就获得了新内存。而 `other` 拿到了旧内存。
* **自动清理旧资源**：函数执行完毕，局部变量 `other` 的生命周期结束，自动调用析构函数，把交换过来的旧内存 `delete[]` 掉。

**总结一句话**：用按值传递，是为了借用编译器生成临时对象和销毁临时对象的机制，自动帮我们完成了深拷贝和旧内存清理。

#### 问题 2：返回类型为什么是 `Buffer&`（返回对象的引用）？

返回类型是 `Buffer&`，返回的语句是 `return *this;`。

* **为什么返回引用 `&`？**
  如果返回值是 `Buffer`（按值返回），编译器需要再执行一次拷贝构造，把当前对象拷贝一份返回，这会产生无谓的巨大开销。赋值结束后，原来的对象还在，所以我们直接返回它的**引用**，效率最高，零拷贝。
* **为什么返回 `*this`？**
  * `this` 是一个隐含的指针，指向当前正在执行操作的对象（在这个例子里就是 `buf1` 的地址）。
  * `*this` 解引用这个指针，得到的就是当前对象本身（即 `buf1` 这个实体）。
  * 返回 `*this` 并加上引用符号 `&`(返回值类型)，意思就是“把赋值后的当前对象本身返回去”。

* **为什么要返回当前对象？**
  为了支持 C++ 标准库的**链式赋值**语法。比如：
  ```cpp
  Buffer a, b, c;
  a = b = c; 
  ```
  这句代码等价于 `a = (b = c);`：
  1. 先执行 `b = c`，把 `c` 赋值给 `b`。
  2. 如果 `operator=` 返回了 `*this`（也就是赋值后的 `b`），那么这个返回值就可以继续作为下一个赋值的右值。
  3. 接着把上一步返回的 `b` 赋值给 `a`。
  如果不返回 `*this`，链式赋值就会编译报错。

##### 5.4. 核心总结

* **传参类型 `Buffer other`**：按值传递，利用编译器制造临时副本，省去手动写深拷贝代码，保证异常安全和自赋值安全。
* **返回类型 `Buffer&`**：返回引用避免多余拷贝；返回 `*this` 支持链式赋值。

这就是 Copy-and-Swap 被称为现代 C++ 资源管理“教科书级惯用法”的原因，短短几行代码包含了极深的内力。



---

## 6. 右值 vs 左值：移动的地基

要讲移动，先得区分**左值（lvalue）**和**右值（rvalue）**。够用即可，别陷进标准里的五种细分。

直觉判断：
- **左值**：有名字、有地址、拷贝后还要继续用的东西。`int x;` 里的 `x`、变量、`*p`。
- **右值**：临时的、马上要消失的、没名字的东西。字面量 `42`、临时对象 `Buffer(100)`、函数按值返回的返回值。

```cpp
int x = 10;
int y = x + 5;   // x+5 的结果是个临时值 -> 右值；x、y 是左值
Buffer makeBuffer();
Buffer b = makeBuffer();  // makeBuffer() 返回的临时 Buffer -> 右值
```

判据小技巧：**能取地址 `&` 的通常是左值**（`&x` 合法），不能取地址的临时量是右值（`&(x+5)` 非法）。

关键洞察：右值是「反正马上就要销毁」的临时对象。既然它马上要死，**我们没必要费劲深拷贝它的资源，直接把它的家当搬过来就行**——这就是移动语义的思想基础。

### 右值引用 `T&&`

C++11 引入了新的引用类型 `T&&`，专门用来**绑定右值**：

```cpp
int&  lref = x;          // 左值引用，只能绑左值
int&& rref = 42;         // 右值引用，绑定右值(临时量)
int&& bad = x;           // 错误：右值引用不能直接绑左值
```

`T&&` 的意义在于**函数重载**：你可以写两个版本，一个收左值 `const T&`，一个收右值 `T&&`，编译器根据实参是左值还是右值自动选。移动构造/移动赋值就是靠这个和拷贝版本区分开的。

| 引用写法 | 绑定什么 | 语义 |
|---|---|---|
| `T&` | 左值 | 可读可改 |
| `const T&` | 左值 + 右值都行 | 只读（拷贝构造用它） |
| `T&&` | 只绑右值 | 「这个可以掏空」（移动用它） |

---

## 6. 左值和右值 （New）

### 一、 基础概念：左值与右值

*   **左值**：占用内存中有可识别地址的数据，可以被取地址（用 `&`），生命周期较长。简单来说，**有名字的变量**基本都是左值。
*   **右值**：不占用持久内存的数据，无法被取地址，生命周期短暂（通常在当前表达式结束后销毁）。例如：字面量（`10`，`3.14`）、临时对象（如函数返回的值、表达式 `x+y` 的结果）。

---

### 二、 引用 `&`（左值引用）

左值引用就是我们常说的“别名”，它必须绑定到一个左值上。

#### 1. 普通左值引用
只能绑定到左值。
```cpp
int a = 10;
int& ref = a; // 正确：a 是左值
// int& ref2 = 10; // 错误：10 是右值，不能绑定到普通左值引用
```

#### 2. 常量左值引用（万能接受者）
`const` 左值引用是一个特例，它可以绑定到右值上（这是 C++ 早期为了处理函数传参拷贝开销而设计的）。
```cpp
const int& ref = 10; // 正确：const 引用可以绑定到右值
const int& ref2 = a; // 正确：也可以绑定到左值
```
**缺点**：通过 `const T&` 绑定右值后，无法修改右值的值。

---

### 三、 右值引用 `&&`

C++11 引入了右值引用，它**只能绑定到右值上**。它的核心目的是**接管即将销毁的临时对象的资源**，从而避免深拷贝。

#### 1. 基本用法
```cpp
int&& ref = 10; // 正确：10 是右值，绑定到右值引用
ref = 20;       // 正确：右值引用可以修改值
// int a = 10;
// int&& ref2 = a; // 错误：a 是左值，不能绑定到右值引用
```

#### 2. 核心价值：移动语义
这是右值引用最重要的应用。在没有右值引用之前，把一个临时对象赋值给另一个对象，需要调用**拷贝构造函数**（深拷贝）。如果对象内部有动态分配的内存，这种拷贝非常耗时，而且临时对象马上就会被销毁，极其浪费。

有了右值引用后，我们可以定义**移动构造函数**和**移动赋值运算符**，直接“窃取”临时对象的内存指针，而不进行深拷贝。

**示例：**
```cpp
class MyString {
    char* data;
    size_t size;
public:
    // 普通拷贝构造（深拷贝，耗时）
    MyString(const MyString& other) {
        size = other.size;
        data = new char[size];
        memcpy(data, other.data, size);
    }

    // 移动构造（浅拷贝，极快）
    MyString(MyString&& other) noexcept { // 注意：参数是右值引用
        data = other.data; // 直接窃取指针
        size = other.size;
        other.data = nullptr; // 置空原对象，防止原对象析构时释放内存
        other.size = 0;
    }
};

MyString createString() {
    MyString temp("hello");
    return temp; // 返回的临时对象是右值
}

int main() {
    // 这里会触发移动构造函数，而不是拷贝构造函数，性能大幅提升
    MyString str = createString(); 
}
```

#### 3. `std::move`：强制变为右值
如果我们要对一个**左值**进行移动操作怎么办？使用 `std::move`。
**注意**：`std::move` 本质上没有任何移动操作，它只是一个类型转换（强制将左值转换为右值引用），从而触发移动构造或移动赋值。
```cpp
MyString a("world");
MyString b = std::move(a); // a 被转换为右值，触发移动构造
// 此后，a 变成了空壳状态，不应再使用 a 的数据
```

---

### 四、 引用折叠 与 完美转发

右值引用的另一个重要应用是模板中的**完美转发**。这里涉及到两个进阶概念：

#### 1. 引用折叠规则
C++ 不允许直接写“引用的引用”（如 `int& &`），但在模板推导和 typedef 中会发生折叠。规则很简单：
*   只要有一个是左值引用（`&`），结果就是左值引用。
*   只有全部是右值引用（`&&`），结果才是右值引用。

#### 2. 模板中的 `T&&`（转发引用 / Universal Reference）
在模板函数中，如果参数写成 `T&&`，它**既能接受左值，也能接受右值**：
*   传入左值，`T` 被推导为 `T&`，`T&&` 折叠为 `T&`。
*   传入右值，`T` 被推导为 `T`，`T&&` 保持为右值引用。

#### 3. `std::forward` 完美转发
如果我们在模板函数中把这个参数传给另一个函数，我们需要保持它原本的左值/右值属性，这就叫完美转发。
```cpp
void process(int& x)  { cout << "左值" << endl; }
void process(int&& x) { cout << "右值" << endl; }

template<typename T>
void wrapper(T&& arg) {
    // std::forward 会保持 arg 的左值/右值属性
    process(std::forward<T>(arg)); 
}

int main() {
    int a = 5;
    wrapper(a);       // 输出: 左值
    wrapper(5);       // 输出: 右值
}
```
*如果在 `wrapper` 中直接调用 `process(arg)`，由于 `arg` 有名字，它本身是个左值，无论传入的是什么，都会调用左值版本的 `process`。使用 `std::forward` 才能完美传递右值属性。*

---

### 五、 总结与对比

| 特性 | 左值引用 `&` | 右值引用 `&&` |
| :--- | :--- | :--- |
| **绑定目标** | 左值（有名字、有地址） | 右值（临时对象、无地址） |
| **是否可修改** | 默认可修改（非 `const` 时） | 默认可修改 |
| **`const` 修饰** | `const T&` 可绑定右值 | 通常不写 `const T&&`（没意义，因为右值引用的目的就是修改/窃取资源） |
| **主要目的** | 避免不必要的参数拷贝，作别名 | **移动语义**（窃取临时对象资源，避免深拷贝）、**完美转发** |
| **典型使用场景** | 函数参数传递、返回引用 | 移动构造函数、移动赋值运算符、`std::move`、模板转发 |

**一句话口诀**：
`&` 是给已有对象起别名，省去拷贝；
`&&` 是给将死（临时）对象续命并掏空它的身体，实现性能飞跃。

---

## 7. 移动语义：偷家而不是复制

移动构造和移动赋值的参数是 `Buffer&&`（右值引用，非 const，因为要改动源对象）。做法是**接管源对象的指针，然后把源对象置空**——不分配、不复制，O(1) 完成。

```cpp
// 移动构造：把 other 的资源直接搬过来
Buffer(Buffer&& other) noexcept
    : size_(other.size_), data_(other.data_) {   // 直接接管 other 的指针
    other.data_ = nullptr;                        // 关键：把 other 置空
    other.size_ = 0;
}

// 移动赋值
Buffer& operator=(Buffer&& other) noexcept {
    if (this == &other) return *this;
    delete[] data_;                // 释放自己的旧资源
    data_ = other.data_;           // 接管 other 的资源
    size_ = other.size_;
    other.data_ = nullptr;         // 掏空 other
    other.size_ = 0;
    return *this;
}
```

### 为什么必须把源对象置空

因为源对象**迟早也要析构**。如果不把 `other.data_` 置空，那么源对象析构时会 `delete[]` 那块已经被新对象接管的内存 → 又是 double free。置空后，`delete[] nullptr` 是安全的空操作（`delete` 空指针 C++ 保证无害）。

### 移动后源对象的状态：有效但未指定

移动完，源对象仍然是个**合法对象**（可以析构、可以重新赋值），但它的**值是未指定的**——你不该再依赖它原来的内容。

```cpp
Buffer a(100);
Buffer b = std::move(a);   // 把 a 移动给 b
// 此刻 a 处于「有效但未指定」状态：a.data_ 是 nullptr
// 你可以 a = Buffer(50); 给它新值，也可以让它自然析构
// 但不该假设 a 还有那 100 个元素
```

> 「有效但未指定（valid but unspecified）」是标准库的约定：移动后源对象能安全析构、能重新赋值，仅此而已。标准库容器（如 `std::string`、`std::vector`）被移动后一般是空的，但别把「空」当成保证去依赖。

---

## 8. `std::move` 到底做了什么（面试高频）

**`std::move` 不移动任何东西。** 它只是一个类型转换，把一个左值**强制转成右值引用**，好让重载决议选中移动版本。名字起得有误导性，记住它约等于 `static_cast<T&&>`。

```cpp
Buffer a(100);
Buffer b = a;             // a 是左值 -> 选拷贝构造（深拷贝，慢）
Buffer c = std::move(a);  // std::move(a) 把 a 转成右值 -> 选移动构造（偷家，快）
```

- `std::move` 本身不产生任何运行时代码，纯编译期的类型转换。
- 真正「搬东西」的是你写的移动构造/移动赋值。`std::move` 只负责「把左值伪装成右值，让编译器愿意调移动版本」。
- 对一个之后还要用的对象别乱 `std::move`，因为它会被掏空。

一句话记忆：**`std::move` = 「我保证不再用这个对象了，你可以放心掏空它」的一个标记，仅此而已。**

### 对 const 对象 move 实际是拷贝（坑）

```cpp
const Buffer a(100);
Buffer b = std::move(a);   // std::move(a) 得到 const Buffer&&
                           // 移动构造参数是 Buffer&&(非const)，绑不上
                           // 只能匹配拷贝构造 const Buffer& -> 实际做了拷贝！
```

因为移动要修改源对象（置空），而 const 对象不许改。所以 `const` 对象是移动不了的，会**静默退回到拷贝**。教训：**别把该移动的对象声明成 `const`。**

---

## 9. 为什么移动对性能重要

用同一个 `Buffer` 对比。假设 `data_` 有一百万个 int（4 MB）：

| 操作 | 拷贝 | 移动 |
|---|---|---|
| 内存分配 | 再分配 4 MB | 不分配 |
| 数据复制 | 复制 100 万个 int | 不复制 |
| 复杂度 | O(n) | O(1)，只改几个指针 |

```cpp
std::vector<Buffer> v;
Buffer big(1000000);
v.push_back(big);              // 拷贝：分配 4MB + 复制 100 万个元素
v.push_back(std::move(big));   // 移动：只搬指针，几乎免费
```

这也是为什么 `std::vector`、`std::string` 这些容器在 C++11 后普遍变快了：往容器里塞临时对象、容器扩容搬迁元素，全都用移动代替了拷贝。返回大对象（如函数返回一个装满数据的 `vector`）也不再有「返回值要深拷贝一遍」的顾虑。

---

## 10. RVO / NRVO：为什么返回局部对象不用手动 `std::move`

**返回值优化（RVO, Return Value Optimization）**：编译器直接在调用者的位置构造返回对象，**连移动都省了**，一次构造到位、零拷贝零移动。

```cpp
Buffer makeBuffer() {
    Buffer local(100);
    return local;      // NRVO：直接在调用处构造 local，不拷贝也不移动
}
Buffer b = makeBuffer();   // b 就是那个 local，没有任何多余的构造
```

- 返回临时量（`return Buffer(100);`）：C++17 起**强制**省略拷贝/移动（保证 RVO）。
- 返回具名局部变量（`return local;`）：NRVO，编译器**允许**但不强制省略；主流编译器基本都会做。

### 别画蛇添足写 `return std::move(local);`

```cpp
Buffer makeBuffer() {
    Buffer local(100);
    return std::move(local);   // 反而更慢！抑制了 NRVO
}
```

原因：`return local;` 时编译器能做 NRVO（零开销）；一旦你写 `std::move(local)`，返回的是个右值引用表达式，NRVO 的条件被破坏，编译器**只能退而求其次做一次移动构造**——你亲手把「零开销」变成了「一次移动」。

> 铁律：**返回局部对象时直接 `return local;`，什么都别加。** 编译器比你会优化。只有极少数情况（如返回一个成员、返回函数参数）编译器无法 NRVO 时，`std::move` 才可能有意义，但那是特例。

---

## 11. noexcept 移动的意义

移动构造/移动赋值应当标记 `noexcept`（承诺不抛异常）。这不只是好习惯，直接影响性能：

```cpp
Buffer(Buffer&& other) noexcept { ... }   // 加 noexcept
```

原因和 `std::vector` 扩容有关。`vector` 满了要重新分配更大的内存，把旧元素搬到新内存。搬的时候它要选：用移动还是拷贝？

- 如果元素的移动构造是 `noexcept`：`vector` 放心用**移动**（快）。
- 如果移动**可能抛异常**：`vector` 为了保证「扩容失败时旧数据不丢」的强异常安全，宁可用**拷贝**（慢但安全）。因为移动到一半抛异常，源数据已经被破坏，没法回滚。

所以**移动没标 noexcept，等于白写了**——`vector` 扩容时根本不会用它，退回慢速拷贝。记住：**移动构造/移动赋值一律加 `noexcept`。**

---

## 12. Rule of 3 / Rule of 5 / Rule of 0

这是本模块的方法论总纲，回答「我到底该写哪几个特殊成员函数」。

### Rule of 3（C++98 时代）
**如果你需要自定义析构、拷贝构造、拷贝赋值三者中的任何一个，那你几乎肯定三个都要写。**

道理：需要自定义析构，说明你在管理某种资源（如 `delete[] data_`）；那么默认的浅拷贝一定是错的（会 double free），所以拷贝构造和拷贝赋值也必须自己写。三者是连坐关系。

### Rule of 5（C++11 起）
加入移动后，变成五个：**析构、拷贝构造、拷贝赋值、移动构造、移动赋值**。一旦你手写了其中任意一个，就该考虑把五个都显式定义（或 `=default`/`=delete`），因为：
- 你手写了拷贝或析构 → 编译器**不再自动生成移动**函数，你的类会退回到「只能拷贝不能移动」，白白损失性能。
- 五个语义要一致，缺一个都可能埋雷。

```cpp
class Buffer {
public:
    Buffer(int n);                                   // 普通构造
    ~Buffer();                                       // 1 析构
    Buffer(const Buffer&);                           // 2 拷贝构造
    Buffer& operator=(const Buffer&);                // 3 拷贝赋值
    Buffer(Buffer&&) noexcept;                       // 4 移动构造
    Buffer& operator=(Buffer&&) noexcept;            // 5 移动赋值
};
```

### Rule of 0（现代 C++ 首选）
**如果你的类不直接管理裸资源，就一个特殊成员函数都别写，让编译器全默认生成。**

做法：把资源交给已经正确实现了 Rule of 5 的类型去持有——`std::vector`、`std::string`、`std::unique_ptr`、`std::shared_ptr`（这些都是 M4 的内容）。你的类只由这些「懂事」的成员组成，编译器默认生成的拷贝/移动会**自动逐成员调用**它们各自正确的拷贝/移动。

```cpp
class Buffer {
public:
    Buffer(int n) : data_(n) {}     // vector 自己会管内存
    // 不写任何特殊成员函数！
    // 拷贝 = vector 深拷贝，移动 = vector 移动，析构 = vector 释放，全自动正确
private:
    std::vector<int> data_;          // 让 vector 去操心 Rule of 5
};
```

对比：

| | Rule of 3/5 | Rule of 0 |
|---|---|---|
| 何时用 | 你在实现底层资源包装类 | 绝大多数业务类 |
| 你写多少代码 | 3 或 5 个特殊成员函数 | 0 个 |
| 出错概率 | 高（手写容易漏 self-assign、noexcept） | 低（复用久经考验的库类型） |
| 现代建议 | 仅在写资源管理原语时 | **默认首选** |

> 记忆链：能用 Rule of 0 就用 Rule of 0；实在要亲手管资源（写智能指针那种底层类）才上 Rule of 5；Rule of 3 是没有移动的老年代版本，理解即可。练习里你会先手写 Rule of 5 的 Buffer（为了搞懂原理），再见识 Rule of 0 版本（为了知道实践中该怎么写）。

---

## 13. `=default` 和 `=delete`

C++11 让你能显式控制特殊成员函数的生成。

### `=default`：要一个「编译器默认版本」，但显式写出来
```cpp
class Buffer {
public:
    Buffer(const Buffer&) = default;             // 明确要默认的逐成员拷贝
    Buffer& operator=(const Buffer&) = default;
    Buffer(Buffer&&) noexcept = default;         // 要默认的逐成员移动
    ~Buffer() = default;
};
```
好处：意图清晰（「我确实想要默认行为」），而且有时你写了别的特殊成员函数导致某个默认版本被抑制，用 `=default` 能把它显式要回来。

### `=delete`：禁止某个操作
```cpp
class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;             // 禁止拷贝
    NonCopyable& operator=(const NonCopyable&) = delete;  // 禁止拷贝赋值
};
NonCopyable a;
NonCopyable b = a;   // 编译错误：拷贝构造被 delete 了
```

### 为什么 unique_ptr 不能拷贝只能移动（面试高频）
`std::unique_ptr` 表达「独占所有权」——一块内存只能有一个 owner。如果允许拷贝，两个 `unique_ptr` 就会指向同一块内存，析构时 double free。所以标准库**把它的拷贝构造和拷贝赋值 `=delete` 了**，只保留移动：

```cpp
std::unique_ptr<int> p1(new int(42));
std::unique_ptr<int> p2 = p1;             // 编译错误：拷贝被 delete
std::unique_ptr<int> p3 = std::move(p1);  // OK：移动，所有权从 p1 转给 p3，p1 变 nullptr
```

这就是「独占语义」的实现方式：**用 `=delete` 禁掉拷贝，用移动转移所有权。** 你自己写只能移动不能拷贝的资源类，也是照这个套路。

---

## 14. 常见坑

1. **有裸指针却用默认拷贝** → 浅拷贝 → double free / 悬垂。有资源就上 Rule of 5，或干脆 Rule of 0 用 vector/智能指针。
2. **拷贝赋值忘了防自赋值** → `a = a;` 时先把自己删了再拷贝自己，读到已释放内存。
3. **拷贝赋值忘了释放旧资源** → 每次赋值泄漏一块内存。
4. **拷贝赋值里先 delete 后 new** → `new` 抛异常时对象已被破坏。应「先分配新的，再删旧的」。
5. **移动构造/赋值忘了把源对象置空** → 源对象析构时 double free。
6. **移动函数没标 `noexcept`** → `vector` 扩容时不敢用移动，退回慢速拷贝，等于白写。
7. **对 const 对象 `std::move`** → 静默退回拷贝，没报错但也没移动。别把要移动的对象声明成 const。
8. **`return std::move(local);`** → 抑制 NRVO，把零开销变成一次移动。返回局部对象直接 `return local;`。
9. **误以为 `std::move` 会搬数据** → 它只是类型转换，真正搬东西的是移动构造/赋值。
10. **手写了拷贝/析构，却忘了移动** → 类退化成「只能拷贝」，性能白丢。要么补全 Rule of 5，要么用 `=default`。
11. **移动后还用源对象的值** → 源对象是「有效但未指定」，别依赖它的内容。

---

## 15. 高频点

- **Rule of 3 / Rule of 5 / Rule of 0 分别是什么？三者关系？** 什么时候必须写全五个？为什么现代 C++ 推荐 Rule of 0？
- **拷贝构造和移动构造的区别？** 参数类型（`const T&` vs `T&&`）、语义（复制 vs 偷家）、源对象是否被改动。
- **`std::move` 到底做了什么？** 只是把左值转成右值引用的类型转换，本身不搬任何东西。
- **什么情况会触发移动？** 实参是右值（临时对象、`std::move(x)`、按值返回的返回值），且存在可用的移动构造/赋值。
- **深拷贝和浅拷贝的区别？** 浅拷贝共享底层内存（危险），深拷贝各持一份。
- **为什么 `unique_ptr` 不能拷贝只能移动？** 独占所有权，拷贝会导致两个 owner 指向同一内存、double free；标准库把拷贝 `=delete` 了。
- **拷贝赋值运算符要注意什么？** 防自赋值、释放旧资源、异常安全（先分配后释放）、返回 `*this`。
- **移动后源对象什么状态？** 有效但未指定，能析构能重新赋值，但别依赖其值。
- **移动构造为什么要 `noexcept`？** 否则 `vector` 扩容时为强异常安全会退回用拷贝。
- **RVO/NRVO 是什么？为什么返回局部对象不该手动 `std::move`？** 编译器能省略构造做到零开销；`std::move` 反而抑制 NRVO。
- **`=default` 和 `=delete` 的用途？**

---

## 16. 编译提醒

单文件练习（x64 Native Tools 命令行）：
```
cl /EHsc /std:c++17 /W4 文件名.cpp
```
mini 项目若拆成多文件：
```
cl /EHsc /std:c++17 /W4 main.cpp Buffer.cpp
```

> 本模块答案在本机是用 `g++ -std=c++17 -Wall -Wextra` 做语法验证的，等价开关。观察特殊成员函数何时被调用时，务必在每个函数里加打印语句——这是理解拷贝/移动最直观的办法。

---

## 17. 承前启后

- **承前（M2）**：拷贝/移动构造就是 M2 学的「构造函数」的特殊重载，析构还是那个析构；`return *this` 在拷贝/移动赋值里再次登场。
- **承前（M4）**：Rule of 0 的底气来自 M4 的 `std::vector`/`std::unique_ptr`/`std::shared_ptr`——它们内部已经实现好了 Rule of 5，你复用即可。RAII 思想在这里体现为「资源的拷贝/移动/释放都绑定到对象上」。
- **启后**：搞懂移动语义后，你才能真正读懂标准库容器为什么快、`emplace_back`/`std::forward`/完美转发（进阶模块）在解决什么问题。移动语义是现代 C++ 性能模型的基石。

下一步：打开 `exercises.md`。前几题手写深拷贝、亲眼看浅拷贝崩溃，中间题手写 Rule of 5 并用打印观察调用时机，最后 mini 项目把五个特殊成员函数全部实现并测试。

