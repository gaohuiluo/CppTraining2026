#include "wav_reader.h"
#include "byte_reader.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>

const char* toString(WavError e) {
    switch (e) {
        case WavError::CannotOpen:        return "无法打开文件";
        case WavError::NotRiff:           return "不是 RIFF 文件";
        case WavError::NotWave:           return "不是 WAVE 文件";
        case WavError::NoFmtChunk:        return "缺少 fmt 块";
        case WavError::NoDataChunk:       return "缺少 data 块";
        case WavError::UnsupportedFormat: return "不支持的格式(仅支持 16-bit PCM)";
    }
    return "未知错误";
}

namespace {
// 把整个文件读进字节 vector。ifstream 本身是 RAII：离开作用域自动关闭(M4)。
std::optional<std::vector<uint8_t>> readAllBytes(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::nullopt;
    // 定位到末尾拿到大小，再读回来
    f.seekg(0, std::ios::end);
    std::streamoff size = f.tellg();
    f.seekg(0, std::ios::beg);
    if (size <= 0) return std::vector<uint8_t>{};
    std::vector<uint8_t> buf(static_cast<size_t>(size));
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}
} // namespace

std::optional<WavReader> WavReader::load(const std::string& path) {
    auto maybeBytes = readAllBytes(path);
    if (!maybeBytes) { std::cerr << toString(WavError::CannotOpen) << "\n"; return std::nullopt; }
    const std::vector<uint8_t>& b = *maybeBytes;

    // --- 校验 RIFF/WAVE 头 ---
    if (b.size() < 12) { std::cerr << toString(WavError::NotRiff) << "\n"; return std::nullopt; }
    if (readTag(b, 0) != "RIFF") { std::cerr << toString(WavError::NotRiff) << "\n"; return std::nullopt; }
    if (readTag(b, 8) != "WAVE") { std::cerr << toString(WavError::NotWave) << "\n"; return std::nullopt; }

    WavReader w;
    bool haveFmt = false, haveData = false;
    std::vector<uint8_t> rawData;   // data 块原始字节

    // --- 遍历块：从偏移 12 开始，不写死偏移量(能跳过未知块) ---
    size_t pos = 12;
    while (pos + 8 <= b.size()) {
        std::string id = readTag(b, pos);
        uint32_t chunkSize = readLE<uint32_t>(b, pos + 4);
        size_t body = pos + 8;

        if (id == "fmt ") {
            if (body + 16 <= b.size()) {
                w.format_.audioFormat   = readLE<uint16_t>(b, body + 0);
                w.format_.numChannels   = readLE<uint16_t>(b, body + 2);
                w.format_.sampleRate    = readLE<uint32_t>(b, body + 4);
                w.format_.byteRate      = readLE<uint32_t>(b, body + 8);
                w.format_.blockAlign    = readLE<uint16_t>(b, body + 12);
                w.format_.bitsPerSample = readLE<uint16_t>(b, body + 14);
                haveFmt = true;
            }
        } else if (id == "data") {
            size_t n = std::min<size_t>(chunkSize, b.size() - body);
            rawData.assign(b.begin() + body, b.begin() + body + n);
            haveData = true;
        }
        // 跳到下一块；块大小为奇数时有 1 字节填充
        pos = body + chunkSize + (chunkSize & 1);
    }

    if (!haveFmt)  { std::cerr << toString(WavError::NoFmtChunk) << "\n";  return std::nullopt; }
    if (!haveData) { std::cerr << toString(WavError::NoDataChunk) << "\n"; return std::nullopt; }
    if (w.format_.audioFormat != 1 || w.format_.bitsPerSample != 16) {
        std::cerr << toString(WavError::UnsupportedFormat) << "\n";
        return std::nullopt;
    }

    // 把 data 原始字节按小端解读成 int16_t 采样
    size_t count = rawData.size() / 2;
    w.samples_.resize(count);
    for (size_t i = 0; i < count; ++i) {
        uint16_t u = readLE<uint16_t>(rawData, i * 2);
        w.samples_[i] = static_cast<int16_t>(u);   // 位模式转有符号
    }

    return w;   // 靠移动/RVO 返回，不手动 std::move(M5)
}

double WavReader::durationSeconds() const {
    if (format_.sampleRate == 0 || format_.numChannels == 0) return 0.0;
    size_t frames = samples_.size() / format_.numChannels;
    return static_cast<double>(frames) / format_.sampleRate;
}

std::ostream& operator<<(std::ostream& os, const WavReader& w) {
    os << "=== WAV 信息 ===\n"
       << "声道数    : " << w.format_.numChannels << "\n"
       << "采样率    : " << w.format_.sampleRate << " Hz\n"
       << "位深      : " << w.format_.bitsPerSample << " bit\n"
       << "采样总数  : " << w.samples_.size() << "\n"
       << "时长      : " << w.durationSeconds() << " 秒";
    return os;
}

// --- 音频分析(M7：算法 + lambda) ---
int16_t peakAmplitude(const std::vector<int16_t>& samples) {
    if (samples.empty()) return 0;
    // 按绝对值比较找峰值
    auto it = std::max_element(samples.begin(), samples.end(),
        [](int16_t a, int16_t b) { return std::abs(a) < std::abs(b); });
    return static_cast<int16_t>(std::abs(*it));
}

double rms(const std::vector<int16_t>& samples) {
    if (samples.empty()) return 0.0;
    double sumSq = 0.0;
    for (int16_t s : samples) sumSq += static_cast<double>(s) * s;
    return std::sqrt(sumSq / samples.size());
}
