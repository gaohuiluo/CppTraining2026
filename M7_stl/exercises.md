# M7 练习

> 编译（x64 Native Tools 命令行）：
> ```
> cl /EHsc /std:c++17 /W4 文件名.cpp
> ```
> 每题先自己写，跑通再看 `answers/`。STL 全是模板，头文件即实现，单文件编译即可。

难度：⭐ 入门　⭐⭐ 巩固　⭐⭐⭐ 综合

---

## 练习 1 ⭐ vector 基本操作
**文件：** `ex1_vector_basics.cpp`

1. 建一个 `std::vector<int>`，用 `push_back` 塞入 1~5。
2. 用**下标**打印一遍，再用**范围 for**打印一遍。
3. 打印 `size()`。
4. 用 `at(2)` 访问第 3 个元素（体会带越界检查的访问）。
5. 在末尾 `pop_back()` 一个，再打印。

**练什么：** vector 的基本增删查、下标 vs 范围 for、`size`。

---

## 练习 2 ⭐ 观察扩容
**文件：** `ex2_capacity.cpp`

1. 建一个空 `std::vector<int>`。
2. 循环 `push_back` 20 次，每次打印 `size()` 和 `capacity()`，观察 capacity 的成倍跳变。
3. 另建一个 vector，先 `reserve(20)`，再循环 push_back 20 次，打印 capacity，确认它**不再跳变**。
4. 注释里写清楚：为什么 reserve 后 capacity 不变？成倍扩容和每次 +1 相比好在哪？

**练什么：** `size` vs `capacity`、扩容机制、`reserve` 的意义。

---

## 练习 3 ⭐⭐ sort + lambda（对比 C 的 qsort）
**文件：** `ex3_sort_lambda.cpp`

1. 建 `std::vector<int> v{5, 2, 8, 1, 9, 3};`。
2. 用 `std::sort` **升序**排序并打印。
3. 用 `std::sort` + lambda **降序**排序并打印。
4. 按"绝对值"排序：先把 v 改成含负数 `{-5, 2, -8, 1, 9, -3}`，用 lambda 按 `abs` 升序排。
5. 注释里对比：C 的 `qsort` 要写一个 `int cmp(const void*, const void*)` 函数、还要 `void*` 强转；这里 lambda 直接写在调用点，且类型安全。

**练什么：** `std::sort`、lambda 作为比较器、vs C 的 qsort。

---

## 练习 4 ⭐⭐ find / find_if / count_if
**文件：** `ex4_find.cpp`

1. 建 `std::vector<int> v{4, 7, 2, 9, 4, 1, 4};`。
2. 用 `std::find` 找值 9，用 `it != v.end()` 判断是否找到，找到就打印它的下标（`it - v.begin()`）。
3. 用 `std::find_if` + lambda 找第一个大于 5 的元素并打印。
4. 用 `std::count` 统计 4 出现几次。
5. 用 `std::count_if` + lambda 统计偶数个数。

**练什么：** 查找系算法、谓词 lambda、"没找到"的判断惯用法。

---

## 练习 5 ⭐⭐ accumulate / transform / max_element
**文件：** `ex5_algorithms.cpp`

1. 建 `std::vector<int> v{3, 1, 4, 1, 5, 9, 2, 6};`。
2. 用 `std::accumulate` 求和（记得 `#include <numeric>`）。
3. 用 `std::accumulate` + lambda 求**平方和**。
4. 用 `std::transform` 把每个元素翻倍，结果写回 v（或写到新 vector），打印。
5. 用 `std::max_element` / `std::min_element` 找最大最小值并打印。
6. 注释：为什么求 double 容器的和时初值必须写 `0.0` 而不是 `0`？

**练什么：** `<numeric>` 与 `<algorithm>` 常用算法、accumulate 的自定义累加器、初值类型陷阱。

---

## 练习 6 ⭐⭐ map 与 unordered_map
**文件：** `ex6_map.cpp`

1. 用 `std::map<std::string, int>` 存 3 个人的年龄，用 `operator[]` 赋值。
2. 范围 for + 结构化绑定 `for (const auto& [name, age] : m)` 遍历，观察输出**按 key 有序**。
3. 演示 `map[]` 的坑：查一个不存在的 key 后，打印 `size()`，确认它被"顺手插入"了。再用 `find` 做一次**只读**查询做对比。
4. 把同样的数据放进 `std::unordered_map`，遍历，注释说明它的顺序**不保证**。
5. 注释里写一句：什么时候选 map、什么时候选 unordered_map。

**练什么：** 关联容器的增查遍历、`map[]` 副作用坑、有序 vs 无序。

---

## 练习 7 ⭐⭐ 迭代器失效与 erase-remove
**文件：** `ex7_erase.cpp`

1. 建 `std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};`。
2. **错误示范**（写在注释里，不要真运行）：说明 `for(it...) if(*it%2==0) v.erase(it);` 为什么是未定义行为。
3. **正确写法 A**：用 `it = v.erase(it)` 接住返回值的循环，删掉所有偶数。
4. **正确写法 B**：用 erase-remove 惯用法 `v.erase(std::remove_if(...), v.end())` 删掉所有偶数，效果一样。
5. 两种写法各跑一份数据，打印结果确认一致。

**练什么：** 迭代器失效、边遍历边删的正确姿势、erase-remove 惯用法。

---

## 练习 8 ⭐⭐ lambda 捕获
**文件：** `ex8_lambda_capture.cpp`

1. 定义 `int threshold = 5;`，用 `count_if` + **值捕获** `[threshold]` 统计 v 中大于 threshold 的个数。
2. 定义 `int sum = 0;`，用 `for_each` + **引用捕获** `[&sum]` 把 v 的元素累加进 sum，打印。
3. 写一个 `mutable` lambda：值捕获一个计数器 `count`，每次调用返回 `++count`；调用几次看它内部累加，且外部 count 不变。
4. 注释里区分：值捕获和引用捕获分别在什么场景用，引用捕获有什么风险（悬空）。

**练什么：** 捕获列表（值/引用/混合）、`mutable`、捕获的语义与风险。

---

## 练习 9 ⭐⭐⭐ std::function 做回调
**文件：** `ex9_function.cpp`

1. 声明 `std::function<int(int)>`，先装一个 lambda（如 `x -> x*x`），调用打印。
2. 再让它装一个普通函数（自己定义一个 `int negate(int)`），调用打印。
3. 写一个函数 `void repeat(int n, const std::function<void(int)>& action)`，内部循环 n 次、每次把当前下标 i 传给 action。用它跑一个打印 lambda。
4. 建 `std::vector<std::function<void()>> tasks;`，塞入 2~3 个不同的 lambda，遍历依次执行。
5. 注释：`std::function` 相比直接用 lambda 的代价是什么？什么时候才需要它？

**练什么：** `std::function` 存储异构可调用对象、作为回调参数、放进容器。

---

## 练习 10 ⭐⭐⭐ 组合实战：学生成绩处理
**文件：** `ex10_students.cpp`

给定 `struct Student { std::string name; int score; };` 和一组数据：

1. 用 `std::sort` + lambda 按 `score` **降序**排列。
2. 用 `std::count_if` 统计及格（score >= 60）人数。
3. 用 `std::accumulate` + lambda 求平均分（先求和再除以人数，注意类型）。
4. 用 `std::find_if` 找出第一个不及格的学生并打印其名字。
5. 用 `std::max_element` + lambda 找最高分的学生。
6. 用 `std::transform` 生成一个 `std::vector<std::string>`，只含所有人的名字，打印。

**练什么：** 容器 + 多种算法 + lambda 的综合运用，模拟真实业务处理。

---

## 综合项目 mini ⭐⭐⭐ 词频统计器
**文件：** `mini/word_freq.cpp`

做一个**词频统计器**，把 STL 的容器、算法、lambda 串起来：

1. 用一段内置的多行字符串当输入文本（用 `std::istringstream` 按空白切词，省去读文件）。也可以选择从 `std::cin` 读，二选一即可。
2. 用 `std::unordered_map<std::string, int>` 统计每个单词出现次数（体会哈希表 O(1) 计数）。
3. 做一点归一化：把单词转小写、去掉首尾标点（用 `std::transform` + `::tolower`，标点用 `std::ispunct` 判断），让 "The" 和 "the"、"word." 和 "word" 归到一起。
4. 把 map 的内容倒进 `std::vector<std::pair<std::string,int>>`。
5. 用 `std::sort` + lambda 按**频率降序**排序；频率相同的按单词**字典序升序**（稳定的次要排序键）。
6. 输出出现次数最多的 **Top N**（N 由常量或参数给定，比如 10；不足 N 就全出）。
7. 顺带用 `std::accumulate` 打印总词数（所有频率之和）和不同单词数（map 的 size）。

**练什么：** 把整个 M7 融会贯通——unordered_map 计数、pair、vector、sort + 多键 lambda、transform、accumulate，一个真正实用的小工具。

---

做完告诉我，或对照 `answers/`。想深入某个点（比如自定义类型怎么做 unordered_map 的 key、`std::sort` 稳定性、或 `std::stable_sort`）随时说。下一模块会进入更工程化的主题，STL 会是你之后所有代码的底座。
