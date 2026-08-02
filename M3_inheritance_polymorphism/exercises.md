# M3 练习

> 编译（x64 Native Tools 命令行）：
> ```
> cl /EHsc /std:c++17 /W4 文件名.cpp
> ```
> 多文件一起编译：
> ```
> cl /EHsc /std:c++17 /W4 main.cpp Shape.cpp ...
> ```
> 每题先自己写，跑通再看 `answers/`。

难度：⭐ 入门　⭐⭐ 巩固　⭐⭐⭐ 综合

---

## 练习 1 ⭐ 第一个继承
**文件：** `ex1_inherit.cpp`

写一个基类 `Animal` 和一个派生类 `Dog`：
1. `Animal`：`protected` 成员 `name_`（std::string）；构造 `Animal(std::string name)`；公共 const 成员函数 `introduce()`，打印 `我是 name_`。
2. `Dog : public Animal`：构造 `Dog(std::string name, std::string breed)`，**用初始化列表调用基类构造**；新增 const 成员函数 `bark()`，打印 `name_ 汪汪叫`（注意能直接用继承来的 `name_`）。
3. `main` 里创建一个 `Dog`，分别调用 `introduce()` 和 `bark()`。

**练什么：** public 继承语法、is-a、派生类构造调用基类构造、protected 成员的访问。

---

## 练习 2 ⭐ 构造/析构顺序
**文件：** `ex2_order.cpp`

1. `Base`：构造打印 `Base 构造`，析构打印 `Base 析构`。
2. `Derived : public Base`：构造打印 `Derived 构造`，析构打印 `Derived 析构`。
3. `main` 里创建一个局部 `Derived` 对象，观察输出。
4. 在注释里回答：为什么构造是"Base 先"、析构是"Derived 先"？

**练什么：** 构造/析构的顺序与对称性。

---

## 练习 3 ⭐⭐ virtual 与多态（对比有无 virtual）
**文件：** `ex3_polymorphism.cpp`

1. 基类 `Shape`，成员函数 `name()` 返回类型名字符串。
2. 派生类 `Circle`、`Square`，各自覆盖 `name()`。
3. **第一步**：先把 `name()` 写成**非虚**，用 `Shape*` 指向 `Circle`，调用 `name()`，观察输出（会是 "Shape"）。
4. **第二步**：把 `name()` 改成 `virtual`，派生类加 `override`，再跑一遍，观察输出变成 "Circle"。
5. 注释里解释静态绑定 vs 动态绑定的差别。

**练什么：** virtual 是多态的开关；静态绑定 vs 动态绑定；override 的使用。

---

## 练习 4 ⭐⭐ 虚析构函数（看资源泄漏）
**文件：** `ex4_virtual_dtor.cpp`

1. 基类 `Base`，派生类 `Derived`。`Derived` 构造时打印 `Derived 申请资源`，析构时打印 `Derived 释放资源`；`Base` 构造/析构也各打印一句。
2. `Base* p = new Derived; delete p;`
3. **第一步**：先让 `~Base()` 非虚，观察 `delete p` 时 `~Derived()` 是否被调用（不会！）。
4. **第二步**：把 `~Base()` 改成 `virtual`，再跑，观察 `~Derived()` 正确执行。
5. 注释里解释：非虚析构 + 基类指针 delete 会导致什么后果。

**练什么：** 为什么基类析构必须 virtual；不这样做的泄漏后果。

---

## 练习 5 ⭐⭐ 纯虚函数与抽象类
**文件：** `ex5_abstract.cpp`

1. 抽象类 `Drawable`：纯虚函数 `virtual void draw() const = 0;`，虚析构。
2. 两个派生类 `Text`、`Image`，各自实现 `draw()`。
3. 试着写 `Drawable d;`（放注释里说明它编译不过，为什么）。
4. `main` 里用 `Drawable*` 数组（或分别）指向 `Text`、`Image` 对象，循环调用 `draw()`。

**练什么：** 纯虚函数、抽象类不可实例化、接口的表达方式。

---

## 练习 6 ⭐⭐ 对象切片
**文件：** `ex6_slicing.cpp`

1. 基类 `Animal`（虚函数 `speak()`，打印 "animal"）；派生类 `Dog`（覆盖 `speak()`，打印 "woof"）。
2. 演示三种调用：
   - `Dog d; Animal& r = d; r.speak();` → woof（引用，多态）
   - `Animal* p = &d; p->speak();` → woof（指针，多态）
   - `Animal a = d; a.speak();` → animal（**切片**，多态失效）
3. 再写一个 `void byValue(Animal a)` 和 `void byRef(const Animal& a)`，各传一个 `Dog`，对比输出。
4. 注释里解释：切片时丢了什么、为什么多态失效。

**练什么：** 对象切片的成因与后果；多态必须用指针/引用。

---

## 练习 7 ⭐⭐⭐ dynamic_cast 与 RTTI
**文件：** `ex7_dynamic_cast.cpp`

1. 基类 `Employee`（虚析构、虚函数 `role()`）；派生类 `Manager`（新增 `holdMeeting()`）、`Engineer`（新增 `writeCode()`）。
2. 写一个函数 `void act(Employee* e)`：用 `dynamic_cast` 判断 `e` 到底是 Manager 还是 Engineer，是 Manager 就 `holdMeeting()`，是 Engineer 就 `writeCode()`。
3. `main` 里分别传 `Manager` 和 `Engineer` 对象验证。
4. 注释里对比：这里如果用 `static_cast` 会有什么风险？为什么 dynamic_cast 要求基类有虚函数？

**练什么：** dynamic_cast 向下转型、判空、RTTI 前提、对比 static_cast。

---

## 练习 8 ⭐⭐⭐ 菱形继承与 virtual 继承
**文件：** `ex8_diamond.cpp`

1. 基类 `Device`，公共成员 `int id_;` 和函数 `powerOn()`。
2. `Camera` 和 `Phone` 都继承 `Device`。
3. `Smartphone : public Camera, public Phone`。
4. **第一步**：先用**普通继承**，试着在 `main` 里写 `Smartphone s; s.id_ = 1;`（放注释里说明为什么有歧义、编译不过）。
5. **第二步**：把 `Camera`、`Phone` 改成 `virtual public Device`，再试 `s.id_ = 1;`，观察现在只有一份 `Device`、不再歧义。
6. 注释里解释虚继承解决了什么。

**练什么：** 多重继承、菱形问题、virtual 继承的作用。

---

## 综合项目 mini ⭐⭐⭐ 形状面积计算器
**文件：** `mini/shape.h`、`mini/shape.cpp`、`mini/main.cpp`

用一套多态体系实现"形状面积计算器"，把 M3 全部知识点串起来，同时练 M2 的头文件/源文件拆分：

1. `shape.h`：
   - 抽象基类 `Shape`：纯虚函数 `double area() const = 0;`、`std::string name() const = 0;`（纯虚）、虚析构 `virtual ~Shape() = default;`。
   - 派生类 `Circle`（成员 `radius_`）、`Rectangle`（成员 `width_`、`height_`），各自 `override` 两个纯虚函数。
   - 用 `#pragma once`。
2. `shape.cpp`：实现 `Circle`、`Rectangle` 的 `area()` 和 `name()`（面积公式：圆 `πr²`，矩形 `w*h`）。
3. `main.cpp`：
   - 用 `std::vector<std::unique_ptr<Shape>>` 装若干不同形状（`std::make_unique<Circle>(...)` 等）。
   - 遍历容器，通过基类指针**多态调用** `area()` 和 `name()`，打印每个形状的名字和面积。
   - 累加求总面积。
4. 思考题（注释里回答）：
   - 为什么容器要存 `unique_ptr<Shape>` 而不是 `Shape` 或 `std::vector<Shape>`？（提示：抽象类不能实例化 + 对象切片）
   - `Shape` 的析构为什么要 virtual？

编译：`cl /EHsc /std:c++17 /W4 main.cpp shape.cpp`

**练什么：** 抽象基类 + 多态 + 容器存多态对象（智能指针）+ 头文件拆分，一个完整的多态设计。这题为 M4（RAII/智能指针）铺路。

---

做完告诉我，或对照 `answers/`。想深入讲某个点（比如虚表在多重继承下的布局、`std::function` 这类"类型擦除"的运行时多态替代方案）随时说——注意 mini 里用到的 `std::unique_ptr` 正是 M4 的主角，这里先埋个伏笔。
