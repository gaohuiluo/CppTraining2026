// WavReader：解析 PCM WAV 文件（综合 M1-M8）
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <iosfwd>

// fmt 块解析出的音频参数（M2：纯数据聚合用 struct）
struct WavFormat {
    uint16_t audioFormat  = 0;   // 1 = PCM
    uint16_t numChannels  = 0;   // 1=单声道 2=立体声
    uint32_t sampleRate   = 0;   // 如 44100
    uint32_t byteRate     = 0;
    uint16_t blockAlign   = 0;
    uint16_t bitsPerSample = 0;  // 8/16/24/32
};

// 解析失败的原因（M8：枚举类）
enum class WavError {
    CannotOpen,
    NotRiff,
    NotWave,
    NoFmtChunk,
    NoDataChunk,
    UnsupportedFormat,
};

const char* toString(WavError e);   // 错误码转可读文本

class WavReader {
public:
    // 工厂函数：成功返回填充好的对象(包在 optional)，失败返回 nullopt。
    // 让调用方无法忽略失败(M8 optional 的价值)。
    static std::optional<WavReader> load(const std::string& path);

    const WavFormat& format() const { return format_; }               // M1 const 正确性
    const std::vector<int16_t>& samples() const { return samples_; }

    double durationSeconds() const;   // 时长 = 帧数 / 采样率

    // M2：打印音频信息报告
    friend std::ostream& operator<<(std::ostream& os, const WavReader& w);

private:
    WavReader() = default;            // 只能经 load 创建
    // 成员都是可自动拷贝/移动的类型 -> Rule of 0(M5)，无需手写特殊成员函数。
    WavFormat format_{};
    std::vector<int16_t> samples_;
};

// 音频分析（M7：算法 + lambda）。放成自由函数，作用在采样上。
int16_t peakAmplitude(const std::vector<int16_t>& samples);
double  rms(const std::vector<int16_t>& samples);
