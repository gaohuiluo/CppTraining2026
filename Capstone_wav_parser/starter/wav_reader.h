// WavReader 起始骨架 —— 填 TODO，卡住了再看 reference/
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <iosfwd>

// fmt 块参数（M2：数据聚合用 struct）
struct WavFormat {
    uint16_t audioFormat   = 0;
    uint16_t numChannels   = 0;
    uint32_t sampleRate    = 0;
    uint32_t byteRate      = 0;
    uint16_t blockAlign    = 0;
    uint16_t bitsPerSample = 0;
};

// 解析失败原因（M8：枚举类）
enum class WavError {
    CannotOpen, NotRiff, NotWave, NoFmtChunk, NoDataChunk, UnsupportedFormat,
};
const char* toString(WavError e);

class WavReader {
public:
    // TODO(阶段1-3): 实现工厂函数 —— 读文件、校验头、遍历块、解析 fmt 和 data。
    static std::optional<WavReader> load(const std::string& path);

    const WavFormat& format() const { return format_; }
    const std::vector<int16_t>& samples() const { return samples_; }

    // TODO(阶段3): 时长 = 帧数 / 采样率
    double durationSeconds() const;

    // TODO(阶段2): 打印音频信息报告
    friend std::ostream& operator<<(std::ostream& os, const WavReader& w);

private:
    WavReader() = default;         // Rule of 0：不写特殊成员函数
    WavFormat format_{};
    std::vector<int16_t> samples_;
};

// TODO(阶段4): 音频分析（M7 算法 + lambda）
int16_t peakAmplitude(const std::vector<int16_t>& samples);
double  rms(const std::vector<int16_t>& samples);
