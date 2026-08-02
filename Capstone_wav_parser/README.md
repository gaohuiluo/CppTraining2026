# 收尾项目：WAV 音频文件解析器

> 这是整个 C++ 学习路径的收尾。目标是**零依赖**（只用标准库）手写一个 WAV 文件解析器，把 M1–M8 学的东西全部用上一遍。做完它，你就具备了「用 C++ 封装二进制格式 + 管理资源」的能力——这正是踏进音视频领域的第一块敲门砖（FFmpeg、编解码器本质都是在解析/搬运这样的二进制数据）。

---

## 为什么选 WAV

- **零依赖**：WAV 是最简单的音频容器格式，纯手写就能解析，不用装任何库，当天能跑起来。
- **概念全覆盖**：正好能自然用到你学过的每一块（见下表）。
- **贴近音视频本质**：音视频的核心就是「解析容器头 → 定位数据块 → 按格式解读字节 → 处理采样/帧」。WAV 是这个套路的最小完整样本。

| 用到的知识 | 来自模块 |
|---|---|
| `iostream`/`fstream`、`std::string`、引用、`const` 正确性 | M1 |
| 类封装、构造/析构、`operator<<`、头文件拆分 | M2 |
| （可选扩展）用继承抽象「音频源」接口 | M3 |
| RAII 管理文件句柄、`unique_ptr` | M4 |
| 用移动语义搬运大块采样缓冲、Rule of 0 | M5 |
| 模板化的「按类型读小端整数」读取函数 | M6 |
| `std::vector` 存采样、`<algorithm>` 求峰值/RMS、lambda | M7 |
| `std::optional` 表示解析失败、`enum class`、异常 vs 错误码、`constexpr` | M8 |

---

## WAV 格式速览（你需要懂的最小知识）

WAV 是 RIFF 容器，由若干「块（chunk）」组成。最简单的 PCM WAV 结构：

```
偏移  大小  字段              说明
0     4    "RIFF"            固定标识（大端字符）
4     4    ChunkSize         整个文件大小 - 8（小端 uint32）
8     4    "WAVE"            固定标识
--- fmt 子块 ---
12    4    "fmt "            注意末尾有个空格
16    4    Subchunk1Size     fmt 块大小，PCM 通常是 16
20    2    AudioFormat       1 = PCM（小端 uint16）
22    2    NumChannels       声道数：1=单声道 2=立体声
24    4    SampleRate        采样率，如 44100
28    4    ByteRate          = SampleRate * NumChannels * BitsPerSample/8
32    2    BlockAlign        = NumChannels * BitsPerSample/8
34    2    BitsPerSample     位深：8/16/24/32
--- data 子块 ---
36    4    "data"            数据块标识
40    4    Subchunk2Size     采样数据字节数
44    ...  采样数据           实际的 PCM 采样
```

> ⚠️ 现实中 fmt 和 data 之间可能夹着别的块（如 `LIST`、`fact`）。健壮的解析器要**循环遍历块、按 id 跳转**，而不是写死偏移量。我们的实现会这么做。

多字节整数在 WAV 里是**小端（little-endian）**存储——这正好用得上 M6 的模板读取函数。

---

## 建议的分阶段做法

不要一口气写完，按下面的里程碑推进，每步都能编译运行看到结果：

**阶段 1：能打开文件、读出并校验 RIFF/WAVE 头**
- 用 RAII（或 `std::ifstream` 本身就是 RAII）打开文件。
- 读前 12 字节，检查 `"RIFF"` 和 `"WAVE"` 标识。
- 失败用 `std::optional` 或异常表达。

**阶段 2：解析 fmt 块，打印音频参数**
- 遍历块，找到 `"fmt "`，解析出声道数、采样率、位深等。
- 打印一份「音频信息报告」。

**阶段 3：读取 data 块的采样数据到 vector**
- 找到 `"data"` 块，把采样读进 `std::vector`。
- 对 16-bit PCM，按 `int16_t` 解读。

**阶段 4：做点音频分析（把 STL/算法用起来）**
- 计算峰值振幅（`std::max_element` + lambda）。
- 计算 RMS（响度）、时长（采样数 / 采样率）。
- 打印分析报告。

**阶段 5（可选扩展）：**
- 用 M3 的多态：定义抽象 `AudioSource` 接口，`WavFile` 实现它，为将来接入别的格式（AIFF/裸 PCM）留口子。
- 用 M8 的 CMake 组织工程 + 写几个断言测试。
- 生成一个测试用的正弦波 WAV 文件（写 WAV），再读回来验证——读写闭环。

---

## 目录内容

```
Capstone_wav_parser/
├── README.md            本文件：项目说明 + WAV 格式 + 分阶段指引
├── SPEC.md              详细需求规格（你要实现的类和函数清单）
├── starter/             起始骨架（带 TODO，你来填）
│   ├── wav_reader.h
│   ├── wav_reader.cpp
│   ├── main.cpp
│   └── CMakeLists.txt
├── reference/           完整参考实现（卡住了再看）
│   ├── byte_reader.h    模板化小端读取（M6）
│   ├── wav_reader.h
│   ├── wav_reader.cpp
│   ├── main.cpp
│   └── CMakeLists.txt
└── tools/
    └── make_test_wav.cpp   生成一个测试 WAV（正弦波），无输入文件时先跑它
```

---

## 怎么开始

1. 先读 `SPEC.md`，看清楚要实现哪些东西。
2. 没有 WAV 文件测试？先编译并运行 `tools/make_test_wav.cpp`，它会生成一个 `test.wav`（1 秒 440Hz 正弦波，16-bit 单声道）。
3. 打开 `starter/`，从阶段 1 开始填 TODO。每完成一个阶段就编译运行。
4. 卡住了看 `reference/`——但建议先自己撞一撞墙，那样记得牢。

编译（命令行，x64 Native Tools）：
```
cl /EHsc /std:c++17 /W4 main.cpp wav_reader.cpp
```
或用 CMake（M8 学过）：
```
cmake -B build -S .
cmake --build build
```

---

做完这个项目，恭喜——C++ 基础阶段正式结课。接下来就可以进入真正的音视频：装 FFmpeg，用你现在的 RAII + 智能指针 + 移动语义功底，去封装 `AVFormatContext`/`AVCodecContext` 那些 C 句柄。那时遇到不懂的 C++ 特性，回头翻对应模块即可。
