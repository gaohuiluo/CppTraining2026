// 生成一个测试用 WAV：1 秒 440Hz 正弦波，16-bit 单声道，44100Hz。
// 没有现成 WAV 文件时先跑它，得到 test.wav 供解析器测试。
// 编译：cl /EHsc /std:c++17 /W4 make_test_wav.cpp
// 运行：make_test_wav   (会在当前目录生成 test.wav)
#include <fstream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <iostream>

namespace {
// 按小端把整数写进字节缓冲
template <typename T>
void putLE(std::vector<uint8_t>& out, T v) {
    for (size_t i = 0; i < sizeof(T); ++i)
        out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
}
void putTag(std::vector<uint8_t>& out, const char* tag) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>(tag[i]));
}
} // namespace

int main() {
    const uint32_t sampleRate = 44100;
    const uint16_t channels   = 1;
    const uint16_t bits       = 16;
    const double   freq       = 440.0;   // A4
    const double   seconds    = 1.0;
    const uint32_t numSamples = static_cast<uint32_t>(sampleRate * seconds);

    // 先生成采样
    std::vector<uint8_t> data;
    for (uint32_t i = 0; i < numSamples; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double s = std::sin(2.0 * 3.14159265358979 * freq * t);
        int16_t sample = static_cast<int16_t>(s * 30000);   // 留点余量不削顶
        putLE<uint16_t>(data, static_cast<uint16_t>(sample));
    }

    const uint32_t byteRate   = sampleRate * channels * bits / 8;
    const uint16_t blockAlign = channels * bits / 8;
    const uint32_t dataSize   = static_cast<uint32_t>(data.size());

    std::vector<uint8_t> file;
    putTag(file, "RIFF");
    putLE<uint32_t>(file, 36 + dataSize);   // ChunkSize
    putTag(file, "WAVE");
    // fmt 子块
    putTag(file, "fmt ");
    putLE<uint32_t>(file, 16);              // PCM 的 fmt 块大小
    putLE<uint16_t>(file, 1);               // AudioFormat = PCM
    putLE<uint16_t>(file, channels);
    putLE<uint32_t>(file, sampleRate);
    putLE<uint32_t>(file, byteRate);
    putLE<uint16_t>(file, blockAlign);
    putLE<uint16_t>(file, bits);
    // data 子块
    putTag(file, "data");
    putLE<uint32_t>(file, dataSize);
    file.insert(file.end(), data.begin(), data.end());

    std::ofstream out("test.wav", std::ios::binary);
    if (!out) { std::cerr << "无法创建 test.wav\n"; return 1; }
    out.write(reinterpret_cast<const char*>(file.data()),
              static_cast<std::streamsize>(file.size()));
    std::cout << "已生成 test.wav (" << file.size() << " 字节)\n";
    return 0;
}
