# WAV 解析器 需求规格

按此清单实现。类型和签名可以微调，但要覆盖列出的功能点。

---

## 1. 数据结构

### `enum class WavError`（M8：枚举类 + 错误处理）
表示解析可能的失败原因：
```cpp
enum class WavError {
    CannotOpen,      // 文件打不开
    NotRiff,         // 缺 "RIFF" 标识
    NotWave,         // 缺 "WAVE" 标识
    NoFmtChunk,      // 找不到 fmt 块
    NoDataChunk,     // 找不到 data 块
    UnsupportedFormat // 非 PCM 或不支持的位深
};
```

### `struct WavFormat`（M2：聚合数据用 struct）
存 fmt 块解析出的参数：
```cpp
struct WavFormat {
    uint16_t audioFormat;    // 1 = PCM
    uint16_t numChannels;    // 1=单 2=立体声
    uint32_t sampleRate;     // 如 44100
    uint16_t bitsPerSample;  // 8/16/24/32
    uint32_t byteRate;
    uint16_t blockAlign;
};
```

---

## 2. 模板化字节读取（M6）

在 `byte_reader.h` 里实现一个从字节流按**小端**读取无符号整数的模板函数：

```cpp
// 从 data[offset] 起，按小端读取 T 类型整数（T = uint16_t / uint32_t）
template <typename T>
T readLE(const std::vector<uint8_t>& data, size_t offset);
```
要求：用 `static_assert` 或 `if constexpr` 保证只接受无符号整数类型；逐字节移位组装，不依赖平台字节序。

---

## 3. 核心类 `WavReader`（M2/M4/M5）

```cpp
class WavReader {
public:
    // 工厂函数：解析成功返回填充好的 WavReader（包在 optional 里），失败返回 nullopt。
    // 这样调用方必须处理失败（呼应 M8 optional）。
    static std::optional<WavReader> load(const std::string& path);

    const WavFormat& format() const;              // M1：const 正确性
    const std::vector<int16_t>& samples() const;  // 16-bit PCM 采样
    double durationSeconds() const;               // 时长 = 采样帧数 / 采样率

    // M2：让对象能被 cout 打印音频信息报告
    friend std::ostream& operator<<(std::ostream& os, const WavReader& w);

private:
    WavReader() = default;                         // 只能通过 load 创建
    WavFormat format_{};
    std::vector<int16_t> samples_;
    // 采样 vector 很大：应支持移动、避免拷贝（M5 Rule of 0，vector 自带移动）
};
```

要点：
- **RAII 读文件**：用 `std::ifstream`（本身是 RAII，析构自动关闭），或自己写 M4 的 RAII 封装。以二进制模式读入全部字节到 `std::vector<uint8_t>`。
- **块遍历**：从偏移 12 开始，循环读 `chunkId(4字节) + chunkSize(4字节)`，按 id 处理 `"fmt "` 和 `"data"`，其余块跳过 `chunkSize` 字节。注意块大小为奇数时有 1 字节填充。
- **Rule of 0**：成员都是能自动拷贝/移动的类型（vector、聚合 struct），不用手写五个特殊成员函数——正是 M5 推荐的做法。返回时靠移动/RVO，不要手动 `std::move`。

---

## 4. 音频分析（M7：STL + 算法 + lambda）

实现下面的分析函数（可作为成员函数或自由函数）：
- `int16_t peakAmplitude(const std::vector<int16_t>&)`：用 `std::max_element` + lambda 找绝对值最大的采样。
- `double rms(const std::vector<int16_t>&)`：均方根，衡量响度。
- 打印报告：声道数、采样率、位深、时长、采样总数、峰值、RMS。

---

## 5. main 程序（M1/M8）

`main.cpp`：
1. 从命令行参数取 WAV 路径（`argv[1]`），没给就提示用法。
2. 调 `WavReader::load`，用结构化绑定或 `if (auto w = ...; w)` 处理 optional（M8）。
3. 成功则 `std::cout << *w` 打印报告 + 打印分析结果；失败打印错误。

---

## 6. 可选扩展

- **M3 多态**：抽象 `AudioSource` 接口（纯虚 `format()`/`samples()`/虚析构），`WavReader` 继承它，为未来别的格式留扩展点。
- **M8 工程化**：写 `CMakeLists.txt`（可执行目标 + C++17），加几个零依赖断言测试（如测 `readLE` 的小端组装、测时长计算）。
- **读写闭环**：实现写 WAV 的函数，配合 `tools/make_test_wav.cpp`，生成→读回→比对。

---

## 验收标准

- 能正确解析标准 16-bit PCM WAV，打印出正确的采样率/声道/时长。
- 对损坏/非 WAV 文件，优雅报错（不崩溃）。
- 块遍历能跳过未知块（不写死偏移 44）。
- 全程 `const` 正确、无裸 `new`/`delete`、无内存泄漏。
