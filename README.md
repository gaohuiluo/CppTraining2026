# C++ 系统学习路径（C 基础 → 现代 C++ → 音视频入门）

面向「有扎实 C 基础、C++ 只有概念认知」的学习者。目标标准 **C++17**，编译器 **Visual Studio / MSVC**。全程突出**与 C 的对比**，偏实践，够用就上手。

## 怎么用

每个模块目录结构一致：
- `principles.md` — 原理精讲（含 vs C 对比表、常见坑、高频面试点）
- `exercises.md` — 练习（难度分级 ⭐/⭐⭐/⭐⭐⭐，约 8 题 + 1 个 mini 项目）
- `answers/` — 每题参考实现，关键行中文注释；mini 项目在 `answers/mini/`

**学习顺序**：M1 → M8 顺序推进，最后做收尾项目。先自己写练习，卡住再看 answers。

**编译方式**（二选一）：
- 命令行：开「x64 Native Tools Command Prompt for VS」，`cl /EHsc /std:c++17 /W4 文件.cpp`（多文件把 .cpp 都列上）
- VS 图形界面：空项目 → 加 .cpp → 项目属性设 C++17 → Ctrl+F5

## 模块地图

| 模块 | 主题 | 核心内容 |
|---|---|---|
| [M1](M1_from_c_to_cpp/) | 从 C 到 C++ | iostream、命名空间、引用、const 正确性、auto/nullptr/范围for、string/vector 初识 |
| [M2](M2_classes_and_objects/) | 类与对象 | 封装、构造/析构、初始化列表、explicit、this、static、友元、运算符重载、头文件拆分 |
| [M3](M3_inheritance_polymorphism/) | 继承与多态 | 虚函数、override/final、纯虚/抽象类、**虚表原理**、菱形继承、dynamic_cast |
| [M4](M4_memory_and_raii/) | 内存与 RAII | new/delete vs malloc、内存错误、RAII、unique_ptr/shared_ptr/weak_ptr、所有权 |
| [M5](M5_copy_and_move/) | 拷贝与移动 | 深浅拷贝、拷贝/移动构造与赋值、std::move、**Rule of 0/3/5**、=default/=delete、RVO |
| [M6](M6_templates/) | 模板与泛型 | 函数/类模板、推导、特化、非类型参数、为何放头文件、看懂 STL 报错 |
| [M7](M7_stl/) | STL 实战 | 容器全家桶、迭代器与失效、常用算法、**lambda**、std::function、erase-remove |
| [M8](M8_modern_cpp_and_engineering/) | 现代 C++ + 工程化 | optional/variant/string_view、结构化绑定、异常、**并发基础**、CMake、单测、Sanitizer |
| [收尾](Capstone_wav_parser/) | WAV 解析器 | 零依赖手写 WAV 解析，串起全部模块，音视频入门第一块敲门砖 |

## 学完之后

收尾项目做完，C++ 基础阶段结课。下一步进入音视频：装 FFmpeg，用你的 RAII + 智能指针 + 移动语义功底去封装 `AVFormatContext`/`AVCodecContext` 这些 C 句柄。遇到不懂的 C++ 特性，回翻对应模块。

---
风格规范见 [_STYLE_GUIDE.md](_STYLE_GUIDE.md)。全部 82 个练习答案已通过 C++17 (`g++ -std=c++17 -Wall -Wextra`) 语法/编译验证。
