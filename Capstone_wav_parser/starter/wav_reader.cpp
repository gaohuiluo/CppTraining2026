#include "wav_reader.h"
// 你需要的头文件（用到再取消注释）：
// #include <fstream>
// #include <iostream>
// #include <algorithm>
// #include <cmath>

const char* toString(WavError e) {
    // TODO: 每个枚举值返回一句中文说明
    (void)e;
    return "TODO";
}

std::optional<WavReader> WavReader::load(const std::string& path) {
    (void)path;
    // 阶段1: 用 ifstream 以二进制读入全部字节到 vector<uint8_t>（ifstream 是 RAII）。
    //        校验前 12 字节：偏移0 == "RIFF"，偏移8 == "WAVE"，否则返回 nullopt。
    //
    // 阶段2: 从偏移 12 开始循环遍历块：
    //        读 4 字节 id + 4 字节小端 size；
    //        遇 "fmt " 解析出 WavFormat 各字段；
    //        遇 "data" 记录数据字节范围；
    //        其余块跳过 size 字节（注意奇数 size 有 1 字节填充）。
    //        提示：写一个模板 readLE<T> 按小端读整数（参考 reference/byte_reader.h，这正是 M6）。
    //
    // 阶段3: 把 data 字节按 int16_t 小端解读进 samples_。
    //        校验 audioFormat==1 且 bitsPerSample==16，否则 UnsupportedFormat。
    //
    // 成功返回填充好的 WavReader（直接 return 对象，靠移动/RVO，别手动 std::move）。
    return std::nullopt;
}

double WavReader::durationSeconds() const {
    // TODO: 帧数 = samples_.size() / numChannels; 时长 = 帧数 / sampleRate
    return 0.0;
}

std::ostream& operator<<(std::ostream& os, const WavReader& w) {
    // TODO: 打印声道/采样率/位深/采样数/时长
    (void)w;
    return os;
}

int16_t peakAmplitude(const std::vector<int16_t>& samples) {
    // TODO: std::max_element + lambda（按绝对值比较）
    (void)samples;
    return 0;
}

double rms(const std::vector<int16_t>& samples) {
    // TODO: 均方根 = sqrt(平方和 / 个数)
    (void)samples;
    return 0.0;
}
