# M3 继承与多态（完整）

> 目标：把"继承"和"多态"这对 C++ OOP 的核心武器讲透。M2 你学会了把「数据 + 操作 + 访问控制」打包成类；M3 要解决两个新问题：一是**代码复用**（新类型直接继承已有类型的能力），二是**统一接口、各自实现**（同一个调用，运行时按对象的真实类型跑不同的代码——这就是多态）。C 里你只能用「struct 嵌套 + 函数指针表 + 手动转型」费劲地模拟这些，C++ 用 `virtual`、虚表把它变成语言内建。

---

## 0. 一句话总览

**继承 = 复用 + 扩展一个已有类型；多态 = 用基类指针/引用调用，运行时自动分派到派生类的实现。**
底层靠**虚表（vtable）+ 虚指针（vptr）**实现。这两句话是整个 M3 的骨架。

---

## 1. 继承：从 C 的"手动模拟"说起

在 C 里，你想让 `Dog` 复用 `Animal` 的字段，只能靠**结构体嵌套 + 手动转型**：

```c
// C 风格：手动模拟继承
typedef struct {
    char name[32];
    int  age;
} Animal;

typedef struct {
    Animal base;      // 把"基类"作为第一个成员放进来
    char   breed[32]; // 派生类新增的字段
} Dog;

void animal_speak(Animal* a) { printf("%s makes a sound\n", a->name); }

Dog d;
strcpy(d.base.name, "Rex");
animal_speak((Animal*)&d);   // 手动转型：因为 base 是第一个成员，地址正好对齐
```

这套做法的痛点：
- 访问基类字段要写 `d.base.name`，层层嵌套，繁琐。
- `(Animal*)&d` 这种转型**靠"base 是第一个成员"的巧合**成立，一旦布局变了就崩，编译器不帮你查。
- 想让 `animal_speak` 对不同动物叫不同的声音？只能自己维护一张**函数指针表**塞进 struct，手动调用——这正是 C++ 虚表的手工版。

C++ 的继承把这些一次性内建：

```cpp
// C++ 风格：语言内建继承
class Animal {
public:
    Animal(std::string name) : name_(std::move(name)) {}
    void speak() const { std::cout << name_ << " makes a sound\n"; }
protected:                          // protected：派生类能访问，外部不能
    std::string name_;
};

class Dog : public Animal {         // Dog "is-a" Animal
public:
    Dog(std::string name, std::string breed)
        : Animal(std::move(name)),  // 显式调基类构造(见第 3 节)
          breed_(std::move(breed)) {}
    void fetch() const { std::cout << name_ << " fetches the ball\n"; }
private:
    std::string breed_;
};

Dog d("Rex", "Husky");
d.speak();   // 直接用基类的成员函数，无需 d.base.speak()
d.fetch();   // 也能用自己新增的
Animal* p = &d;   // 派生类指针能"隐式向上转型"成基类指针，安全、编译器认可
```

对比逐条：

| | C 手动模拟 | C++ 继承 |
|---|---|---|
| 复用基类字段 | `d.base.name` 层层嵌套 | `name_` 直接访问 |
| 向上转型 | `(Animal*)&d` 靠布局巧合 | `Animal* p = &d;` 语言保证安全 |
| 多态分派 | 手写函数指针表 | `virtual` + 编译器生成虚表 |
| 访问保护 | 无，字段全裸露 | `protected`/`private` 分级 |

### is-a 关系（public 继承的语义）

`public` 继承表达的是 **"is-a"（是一种）**：`Dog is-a Animal`。判断该不该用继承，就问一句："派生类的对象，能不能当作基类对象来用而不出问题？" 能，才用 public 继承。

反例：`Stack` 不该继承 `std::vector`——栈不是"一种"可随机访问的动态数组，那是 **"has-a"（有一个）** 或"用它实现"的关系，该用**组合**（把 vector 作为成员）而不是继承。

> 记住一句设计原则：**优先组合，慎用继承。** 继承是最强的耦合，只在真正的 is-a 关系下才值得。

### 链式（多层）继承

链式继承指的是一条继承链，一层接一层。比如：

```Cpp
class A {
public:
    void funcA() { }
};

class B : public A {        // B 继承 A
public:
    void funcB() { }
};

class C : public B {        // C 继承 B，也就间接继承了 A
public:
    void funcC() { }
};
这里的继承链是 A -> B -> C。此时 C 的对象可以访问三层的公有成员：
Cpp
int main() {
    C obj;
    obj.funcA();  // 来自 A（间接继承）
    obj.funcB();  // 来自 B（直接继承）
    obj.funcC();  // 来自 C 自己
    return 0;
}
```

---

## 2. 三种继承方式：public / protected / private

继承时那个 `class Dog : public Animal` 里的 `public` 是**继承方式**，它决定"基类成员在派生类里的访问级别上限"。

| 继承方式 | 基类 `public` 成员变成 | 基类 `protected` 成员变成 | 基类 `private` 成员 |
|---|---|---|---|
| `public`（最常用） | `public` | `protected` | 不可访问 |
| `protected` | `protected` | `protected` | 不可访问 |
| `private`（默认，class） | `private` | `private` | 不可访问 |

要点：
- **基类的 `private` 成员，无论哪种继承方式，派生类都碰不到**（只能通过基类的 public/protected 函数间接访问）。
- **99% 的场景用 `public` 继承**，它保持 is-a 关系。
- `private`/`protected` 继承表达的是"用基类来实现"（implemented-in-terms-of），而不是 is-a——这种需求通常**用组合更清晰**，实际很少用。知道有这回事即可。

```cpp
class Base {
public:    int a;
protected: int b;
private:   int c;   // 派生类永远访问不到 c
};

class Pub : public Base {
    void f() { a = 1; b = 2; /* c = 3; 错误 */ }   // a 仍是 public，b 仍是 protected
};
class Priv : private Base {
    void f() { a = 1; b = 2; }                     // a、b 在 Priv 里都变成 private
};

Pub  p; p.a = 1;   // OK：public 继承，a 对外仍可见
Priv q; // q.a = 1; 错误：private 继承，a 对外不可见了
```

> `struct` 继承默认是 `public`，`class` 继承默认是 `private`。所以你几乎总要显式写 `: public Base`，别漏了 `public`——漏了在 `class` 里就成了 private 继承，向上转型会失败，是个隐蔽坑。

一句话概括：protected 成员对外部隐藏（像 private），但对派生类开放（像 public）。protected 的价值主要体现在继承里。派生类可以直接访问基类的 protected 成员，但无法访问 private 成员。

```c++
class Base {
public:
    int pub = 1;
protected:
    int prot = 2;
private:
    int priv = 3;
};

int main() {
    Base b;
    b.pub;   // ✅ 外部可访问
    b.prot;  // ❌ 编译错误：protected 对外部不可见
    b.priv;  // ❌ 编译错误：private 对外部不可见
}
```

---

## 3. 构造/析构顺序：基类先构造、后析构

派生类对象包含一个"基类子对象"。构造和析构的顺序是固定的、对称的：

- **构造：先基类，后派生类**（先打地基再盖楼）。
- **析构：先派生类，后基类**（先拆楼再拆地基），和构造严格相反。

```cpp
class Base {
public:
    Base()  { std::cout << "Base 构造\n"; }
    ~Base() { std::cout << "Base 析构\n"; }
};
class Derived : public Base {
public:
    Derived()  { std::cout << "Derived 构造\n"; }
    ~Derived() { std::cout << "Derived 析构\n"; }
};

Derived d;
// 输出：
// Base 构造        <- 基类先
// Derived 构造
// (析构逆序)
// Derived 析构     <- 派生类先
// Base 析构
```

### 派生类构造函数怎么初始化基类

派生类构造函数用**初始化列表**调用基类构造函数（回忆 M2 的初始化列表）：

```cpp
class Dog : public Animal {
public:
    Dog(std::string name, std::string breed)
        : Animal(std::move(name)),   // 第一步：显式调基类构造
          breed_(std::move(breed))   // 第二步：初始化自己的成员
    {}
private:
    std::string breed_;
};
```

规则：
- 如果不显式写 `Animal(...)`，编译器会尝试调用基类的**默认构造函数**；基类没有默认构造就编译错误。
- 基类构造总是在派生类成员初始化**之前**完成，无论你在初始化列表里把它写在哪个位置（顺序由前述规则决定，不由书写顺序决定）。

完整初始化顺序：**基类构造 → 派生类成员（按声明顺序）→ 派生类构造函数体**。

---

## 4. `virtual` 虚函数：多态的开关

先看一个"没有 virtual"会发生什么的例子：

```cpp
class Animal {
public:
    void speak() const { std::cout << "some sound\n"; }   // 非虚
};
class Dog : public Animal {
public:
    void speak() const { std::cout << "woof\n"; }         // 想"覆盖"基类
};

Animal* p = new Dog;
p->speak();   // 输出 "some sound"！——不是我们想要的 "woof"
```

因为 `speak` 不是虚函数，`p->speak()` 在**编译期**就根据指针类型 `Animal*` 定死了调用 `Animal::speak`。这叫**静态绑定（早绑定）**。

加上 `virtual`，就变成**动态绑定（晚绑定）**——运行时看指针指向的**真实对象类型**来决定调谁：

```cpp
class Animal {
public:
    virtual void speak() const { std::cout << "some sound\n"; }   // 虚函数
    virtual ~Animal() = default;                                  // 见第 7 节：基类析构必须 virtual
};
class Dog : public Animal {
public:
    void speak() const override { std::cout << "woof\n"; }        // override 覆盖
};

Animal* p = new Dog;
p->speak();   // 输出 "woof"！——运行时分派到 Dog::speak
delete p;
```

**这就是多态（polymorphism）：** 通过基类指针/引用调用虚函数，实际执行的是对象真实类型的版本。

### `override` 关键字（C++11，务必用）

`override` 显式声明"我要覆盖基类的虚函数"。它让编译器帮你检查：如果基类根本没有匹配的虚函数（比如你把参数、const 写错了），直接编译报错。

```cpp
class Animal {
public:
    virtual void speak() const {}
};
class Dog : public Animal {
public:
    void speak() override {}   // 错误！基类的是 const 版，签名不匹配
    //         ^^^^^^^^ 没有 override 的话，这会被当成一个"新函数"，静默失败
};
```

> 惯用法：**所有意图覆盖基类虚函数的地方，一律加 `override`。** 它零成本，能挡掉大量"以为覆盖了其实没有"的 bug。派生类里覆盖的函数，`virtual` 关键字可省（自动继承虚性），但 `override` 要写。

### `final` 关键字（C++11）

`final` 有两个用法：
- 修饰虚函数：禁止更下层的派生类再覆盖它。
- 修饰类：禁止这个类被继承。

```cpp
class Cat : public Animal {
public:
    void speak() const override final {}   // 后代不能再覆盖 speak
};
class SpecialCat final : public Cat {};    // SpecialCat 不能再被继承
// class X : public SpecialCat {};         // 错误
```

用途：明确设计意图 + 给编译器优化空间（`final` 的虚调用可能被"去虚化"成直接调用）。

---

## 5. 静态绑定 vs 动态绑定：多态到底何时发生

这是理解多态的关键。动态分派（多态）**只在同时满足以下条件时发生**：

1. 调用的是**虚函数**（`virtual`）。
2. 通过**基类的指针或引用**调用（不是对象本身）。

任一条件不满足，就是编译期定死的**静态绑定**。

```cpp
Dog d;
Animal& r = d;     // 引用
Animal* p = &d;    // 指针
Animal  v = d;     // 对象(注意：发生了切片，见第 9 节)

r.speak();   // 动态绑定 -> Dog::speak (虚函数 + 引用)
p->speak();  // 动态绑定 -> Dog::speak (虚函数 + 指针)
v.speak();   // 静态绑定 -> Animal::speak！(是对象本身，不是指针/引用)
d.speak();   // 静态绑定 -> Dog::speak (编译期就知道 d 是 Dog，没必要动态)
```

> 记牢那句话：**多态需要"通过基类指针或引用"+"虚函数"。** 用对象本身调用永远是静态绑定，而且往往伴随对象切片。这是面试高频考点。

还有两个静态绑定的陷阱（第 12 节还会强调）：
- 在**构造函数/析构函数里**调用虚函数，不会分派到派生类（那时派生类部分还没构造好/已析构）。
- 加了 `final` 的虚函数可能被去虚化。

---

## 6. 纯虚函数与抽象类（接口）

把虚函数写成 `= 0`，它就是**纯虚函数（pure virtual）**：只声明、不（必）实现，强制派生类去实现。

含有纯虚函数的类叫**抽象类（abstract class）**，它**不能被实例化**，只能作为基类/接口。

```cpp
class Shape {
public:
    virtual double area() const = 0;   // 纯虚函数：Shape 不知道怎么算面积，交给派生类
    virtual void draw() const = 0;
    virtual ~Shape() = default;        // 抽象基类的析构也要 virtual
};

// Shape s;   // 错误：抽象类不能实例化

class Circle : public Shape {
public:
    Circle(double r) : r_(r) {}
    double area() const override { return 3.14159265 * r_ * r_; }   // 必须实现所有纯虚函数
    void draw() const override { std::cout << "Circle\n"; }
private:
    double r_;
};
```

要点：
- 派生类**必须实现基类所有纯虚函数**，否则它自己也是抽象类，也不能实例化。
- 抽象类相当于其他语言里的 **interface（接口）**——定义"能做什么"，不管"怎么做"。C++ 没有单独的 `interface` 关键字，用"全是纯虚函数的抽象类"来表达接口。
- 纯虚函数也**可以有实现体**（`Shape::area` 在类外定义），派生类可用 `Shape::area()` 显式调用它作为公共逻辑，但派生类仍必须 override。这是进阶技巧，先知道即可。

对比 C：C 里的"接口"只能用**函数指针结构体**手工搭（像 Linux 内核的 `file_operations`），C++ 用抽象类把这件事变得类型安全、语言内建。

---

## 7. 为什么基类析构函数要声明为 `virtual`（重中之重）

**结论先行：只要一个类可能被当作基类、并通过基类指针 `delete`，它的析构函数就必须是 `virtual`。**

看不这样做的后果：

```cpp
class Base {
public:
    ~Base() { std::cout << "~Base\n"; }        // 非虚析构 —— 埋雷
};
class Derived : public Base {
public:
    Derived() { buf_ = new int[100]; }
    ~Derived() { delete[] buf_; std::cout << "~Derived\n"; }  // 负责释放资源
private:
    int* buf_;
};

Base* p = new Derived;
delete p;   // 灾难：析构非虚 -> 只调 ~Base，不调 ~Derived！
            // 输出只有 "~Base"，Derived 的 buf_ 泄漏了，行为是未定义的
```

因为 `~Base` 不是虚函数，`delete p` 走**静态绑定**，只按指针类型 `Base*` 调用 `~Base`，**派生类的析构被跳过**——`buf_` 内存泄漏，若派生类持有文件/锁等资源则更严重。这是 UB（未定义行为）。

改成虚析构就对了：

```cpp
class Base {
public:
    virtual ~Base() { std::cout << "~Base\n"; }   // 虚析构
};

Base* p = new Derived;
delete p;   // 动态绑定 -> 先 ~Derived 再 ~Base，正确释放
            // 输出 "~Derived" "~Base"
```

规则总结：
- **有虚函数的类，析构函数几乎一定也要 virtual**（既然打算多态使用，就会通过基类指针 delete）。
- 不打算被继承的类，别加 virtual（虚函数有开销，见下节）。
- `virtual ~Base() = default;` 是最常见写法（要虚性，又不想手写空实现）。

> 面试必问："基类析构为什么要 virtual？" 标准答案就是上面那段：非虚析构 + 基类指针 delete = 派生类析构不执行 = 资源泄漏 + UB。

---

## 8. 虚表（vtable）与虚指针（vptr）：多态的底层原理

面试高频、也是真正理解多态的关键。慢慢看。

### 8.1 编译器做了什么

一旦一个类有虚函数，编译器就为**这个类**生成一张**虚函数表（vtable）**：一个函数指针数组，每个槽位放一个虚函数的地址。同时给**每个对象**塞一个隐藏成员——**虚指针（vptr）**，指向本类的 vtable。

```cpp
class Animal {
public:
    virtual void speak() const { ... }   // vtable 槽 0
    virtual void move() const  { ... }   // vtable 槽 1
    virtual ~Animal() {}
    std::string name_;
};
class Dog : public Animal {
public:
    void speak() const override { ... }  // 覆盖槽 0
    // move 没覆盖，沿用 Animal 的
    int breed_id_;
};
```

### 8.2 内存布局示意（ASCII）

```
        Animal 对象内存                 Animal 的 vtable
      +------------------+           +---------------------------+
      | vptr             |---------->| [0] &Animal::speak        |
      +------------------+           | [1] &Animal::move         |
      | name_            |           | [2] &Animal::~Animal      |
      +------------------+           +---------------------------+


        Dog 对象内存                    Dog 的 vtable
      +------------------+           +---------------------------+
      | vptr             |---------->| [0] &Dog::speak   (覆盖!) |
      +------------------+           | [1] &Animal::move (继承)  |
      | name_            |           | [2] &Dog::~Dog            |
      +------------------+           +---------------------------+
      | breed_id_        |
      +------------------+
```

看这张图能读出几件关键的事：
- **vptr 在对象内存最前面**（典型实现），所以对象大小会比字段总和多一个指针（32 位 4 字节 / 64 位 8 字节）。
- `Dog` 的 vtable 里，槽 0 被换成了 `&Dog::speak`（因为覆盖了），槽 1 还指向 `&Animal::move`（没覆盖，继承）。
- **每个类一张 vtable（全类共享），每个对象一个 vptr（各自持有）。** vptr 在构造时由编译器自动设置成指向本类 vtable。

### 8.3 一次虚调用发生了什么

```cpp
Animal* p = /* 某个 Dog 或 Cat */;
p->speak();
```

编译器把 `p->speak()` 翻译成大致这样（伪代码）：

```
1. vptr  = *p            // 取对象开头的 vptr
2. fnptr = vptr[0]       // 从 vtable 取第 0 槽(speak 的地址)
3. call fnptr(p)         // 调用它，把 p 作为 this 传入
```

关键：**槽位下标 `[0]` 是编译期定死的**（speak 固定在第 0 槽），但**vtable 是运行期才知道的**（取决于 p 指向的真实对象是 Dog 还是 Cat 的 vtable）。这就是"编译期定索引、运行期查表"，多态由此实现。

### 8.4 和 C 手写函数指针表的对比

还记得第 1 节说 C 里模拟多态要"手写函数指针表塞进 struct"？虚表就是**编译器帮你自动做了这件事**：

| | C 手动 | C++ 虚函数 |
|---|---|---|
| 函数指针表 | 你手写、手动维护 | 编译器自动生成 vtable |
| 表指针 | 你手动存进 struct、手动赋值 | 编译器自动插入 vptr、构造时自动设置 |
| 调用 | `obj->vtable->fn(obj)` 手写 | `p->fn()` 编译器翻译 |
| 出错风险 | 忘设指针、下标错位 | 编译器保证正确 |

### 8.5 成本

- **空间**：每个多态对象多一个 vptr；每个多态类多一张 vtable。
- **时间**：每次虚调用多一次"查表取址"的间接跳转，且难以内联。
- 结论：多态不是免费的。**该用就用**（需要运行时多态时），但别给不需要多态的类乱加 virtual。这也是 C++"不为不用的特性付代价"哲学的体现。

---

## 9. 对象切片（object slicing）

**用基类类型的对象（不是指针/引用）去接收一个派生类对象时，派生类"多出来的部分"被切掉。** 这是 C 程序员转 C++ 极易踩的坑。

```cpp
class Animal {
public:
    virtual void speak() const { std::cout << "animal\n"; }
    std::string name_;
};
class Dog : public Animal {
public:
    void speak() const override { std::cout << "woof\n"; }
    std::string breed_;   // Dog 特有
};

Dog d;
d.name_ = "Rex"; d.breed_ = "Husky";

Animal a = d;   // 切片！只拷贝 Animal 那部分，breed_ 丢失
a.speak();      // 输出 "animal"，不是 "woof"——因为 a 就是个纯 Animal 了
```

为什么？`Animal a = d;` 是**按值拷贝**，`a` 是个货真价实的 `Animal` 对象，大小只够装 Animal 部分，`d` 的 `breed_` 和 vptr（指向 Dog vtable 的）都进不来——`a` 的 vptr 被设成指向 Animal 的 vtable。于是多态也没了。

切片还常悄悄发生在**按值传参**：

```cpp
void process(Animal a) { a.speak(); }   // 按值收 -> 一进来就被切片
Dog d;
process(d);   // 输出 "animal"，多态失效
```

**修复：多态一律用基类指针或引用，绝不按值。**

```cpp
void process(const Animal& a) { a.speak(); }   // 引用，不切片
process(d);   // 输出 "woof"，正确
```

> 一句话记忆：**要多态，就用指针或引用（`Base*` / `Base&`）；按值传/存基类对象 = 切片 + 丢多态。** 容器里存多态对象也一样，不能 `std::vector<Animal>`，要 `std::vector<std::unique_ptr<Animal>>`（M4/M5 细讲智能指针）。

---

## 10. 多重继承与菱形问题，`virtual` 继承

C++ 允许一个类同时继承**多个基类**（多重继承，C 完全没有对应概念）：

```cpp
class Camera   { public: void takePhoto() {} };
class Phone    { public: void call() {} };
class Smartphone : public Camera, public Phone {};   // 同时是相机和电话

Smartphone s;
s.takePhoto();
s.call();
```

多重继承能用，但容易出问题，最经典的是**菱形继承（diamond problem）**：

```
        Device
        /     \
   Camera     Phone          <- 两者都继承 Device
        \     /
      Smartphone              <- 同时继承 Camera 和 Phone
```

```cpp
class Device { public: int id_; };
class Camera : public Device {};
class Phone  : public Device {};
class Smartphone : public Camera, public Phone {};

Smartphone s;
// s.id_ = 1;        // 错误！有歧义：是 Camera::Device::id_ 还是 Phone::Device::id_?
s.Camera::id_ = 1;   // 得指明路径，很丑
s.Phone::id_  = 2;   // 而且是两份独立的 id_，逻辑上是错的
```

问题根源：`Smartphone` 里**有两份 `Device` 子对象**（一份来自 Camera，一份来自 Phone），`id_` 有两个副本，产生歧义。

### 解决方案：虚继承（virtual inheritance）

让 Camera 和 Phone **虚继承** Device，这样菱形顶端的 Device 在 Smartphone 里**只保留一份**：

```cpp
class Device { public: int id_; };
class Camera : virtual public Device {};   // virtual 继承
class Phone  : virtual public Device {};   // virtual 继承
class Smartphone : public Camera, public Phone {};

Smartphone s;
s.id_ = 1;   // OK！只有一份 Device::id_，不再有歧义
```

`virtual` 继承让最派生类（Smartphone）负责直接构造那唯一一份虚基类 Device，代价是布局更复杂、有额外开销（对象里要存"虚基类偏移"信息）。

> 实践建议：**多重继承要慎用，菱形结构尽量避免。** 现代 C++ 常见的正当用法是"继承多个纯接口（抽象类）"——比如同时实现 `Drawable` 和 `Serializable` 两个接口，接口没有数据成员，就不会有菱形数据冲突。知道 `virtual` 继承能解菱形即可，日常不必频繁使用。

---

## 11. `dynamic_cast` 与 RTTI 入门；对比 `static_cast`

多态场景下，你有时手里是 `Base*`，想问："它到底是不是 `Derived`？如果是，给我一个 `Derived*`。" 这需要**运行时类型信息（RTTI, Run-Time Type Information）**。

### dynamic_cast：安全的向下转型

```cpp
class Animal { public: virtual ~Animal() = default; };
class Dog : public Animal { public: void fetch() { /*...*/ } };
class Cat : public Animal {};

void handle(Animal* a) {
    Dog* d = dynamic_cast<Dog*>(a);   // 运行时检查 a 是否真的指向 Dog
    if (d) {                          // 是 Dog -> 返回有效指针
        d->fetch();
    } else {                          // 不是 Dog -> 返回 nullptr
        std::cout << "不是狗，跳过\n";
    }
}
```

规则：
- `dynamic_cast<T*>`：转指针，**失败返回 `nullptr`**（所以要判空）。
- `dynamic_cast<T&>`：转引用，失败**抛 `std::bad_cast` 异常**（引用没法为空）。
- **前提：基类必须有虚函数**（否则没有 RTTI 信息，编译报错）。这也是"多态类型才有 vptr"的延伸——RTTI 信息就挂在 vtable 上。

### static_cast vs dynamic_cast

| | `static_cast` | `dynamic_cast` |
|---|---|---|
| 检查时机 | 编译期，**不做运行时检查** | 运行期检查真实类型 |
| 向下转型失败时 | 得到"看起来有效"的错误指针，用起来 UB | 指针返回 nullptr / 引用抛异常 |
| 开销 | 零 | 有运行时查表开销 |
| 要求 | 无 | 类必须多态(有虚函数) |
| 何时用 | 你**确定**类型正确、或数值转换 | 你**不确定**、需要运行时判别 |

```cpp
Animal* a = new Dog;
Dog* d1 = static_cast<Dog*>(a);    // 你担保 a 是 Dog，编译器信你，不检查
Dog* d2 = dynamic_cast<Dog*>(a);   // 运行时验证，安全但有开销

Animal* a2 = new Cat;
Dog* bad = static_cast<Dog*>(a2);  // 编译通过！但 a2 其实是 Cat，bad 用起来就是 UB
Dog* ok  = dynamic_cast<Dog*>(a2); // 返回 nullptr，安全
```

### typeid（RTTI 的另一半）

```cpp
#include <typeinfo>
Animal* a = new Dog;
std::cout << typeid(*a).name() << "\n";   // 运行时得到真实类型名(实现相关，如 "3Dog")
if (typeid(*a) == typeid(Dog)) { /* 确实是 Dog */ }
```

> 实践建议：**dynamic_cast 是"设计气味"信号。** 频繁地"判断类型再分别处理"，往往说明你该用**虚函数**让多态自动分派，而不是手动 `dynamic_cast`。它的正当用途是少数确实需要"按真实类型分支"的场景（如插件系统、序列化）。能用虚函数就别用 dynamic_cast。
>
> 对比 C：C 里没有 RTTI，你只能自己在 struct 里塞一个 `int type_tag` 字段，手动 `switch`。dynamic_cast/typeid 是这套手法的语言内建、类型安全版。

---

## 12. 常见坑（从 C 过来最易踩的）

1. **忘写 `virtual`，以为覆盖了其实没有** → 基类指针调用跑的是基类版本（静态绑定）。用 `override` 让编译器帮你查。
2. **基类析构不是 `virtual`，却通过基类指针 delete** → 派生类析构不执行，资源泄漏 + UB（第 7 节）。
3. **对象切片**：按值把派生对象赋给/传给基类对象 → 派生部分被切掉、多态失效（第 9 节）。要用指针/引用。
4. **漏写 `: public`**：`class D : Base` 在 class 里默认是 `private` 继承，向上转型 `Base* p = &d;` 会失败。
5. **在构造/析构函数里调虚函数** → 不会分派到派生类版本。构造时派生类部分还没建好，析构时已经拆了，此刻对象的"真实类型"就是当前正在构造/析构的这一层。
6. **`override` 签名不匹配却没加 override** → 静默变成新函数（重载而非覆盖），最典型是漏写 `const` 或参数类型不同。
7. **用 `static_cast` 做没把握的向下转型** → 编译能过但运行 UB，该用 `dynamic_cast`。
8. **误用继承表达 has-a**（如 Stack 继承 vector）→ 破坏 is-a 语义。优先组合。
9. **误以为 `protected` 等于"派生类能访问基类对象的 protected 成员"**：实际上派生类只能通过**派生类自己的实例**访问继承来的 protected 成员，不能访问"另一个基类对象"的 protected 成员。细节，遇到再说。

---

## 13. 高频面试点（M3 相关）

- 什么是多态？发生的两个条件是什么？（虚函数 + 基类指针/引用）
- 虚函数是怎么实现的？讲讲 vtable 和 vptr。（第 8 节，能画出内存布局最好）
- vptr 存在哪、什么时候设置？一个类几张 vtable、一个对象几个 vptr？（对象开头；构造时设置；每类一张表、每对象一个 vptr）
- 为什么基类析构函数要声明为 virtual？不这样会怎样？（第 7 节标准答案）
- 静态绑定和动态绑定的区别？分别什么时候发生？
- 纯虚函数、抽象类是什么？抽象类能实例化吗？
- 什么是对象切片？怎么避免？
- 构造/析构的顺序？为什么析构是逆序？
- 构造函数/析构函数里能调用虚函数吗？会发生什么？（能调，但不会动态分派到派生类）
- `override` 和 `final` 的作用？
- `dynamic_cast` 和 `static_cast` 的区别？dynamic_cast 的前提是什么？
- 什么是菱形继承？virtual 继承怎么解决？
- public / protected / private 三种继承的区别？
- 什么时候用继承、什么时候用组合？（is-a 用继承，has-a 用组合，优先组合）

---

## 14. 编译提醒

单文件练习（x64 Native Tools 命令行）：
```
cl /EHsc /std:c++17 /W4 文件名.cpp
```
多文件（头文件 + 多个源文件）一起编译：
```
cl /EHsc /std:c++17 /W4 main.cpp Shape.cpp Circle.cpp
```

---

前置回顾：M3 大量用到 M2 的**初始化列表**（派生类调基类构造）、**const 成员函数**（虚函数也要保持 const 正确性）、**访问控制**（`protected` 在这里终于派上用场）。

后续伏笔：M4 讲 **RAII 与动态内存/智能指针**——你会明白为什么多态对象常常要用 `std::unique_ptr<Base>` 来持有（既要多态又要自动释放，还避免切片）；M5 讲**拷贝控制**（拷贝构造/赋值），到时会解释"切片"背后的拷贝语义，以及多态类型为什么常常禁用拷贝。下一步：打开 `exercises.md`。
