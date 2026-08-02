// WAV 解析器 主程序
// 编译：cl /EHsc /std:c++17 /W4 main.cpp wav_reader.cpp
#include "wav_reader.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "用法: " << argv[0] << " <文件.wav>\n"
                  << "提示: 可先编译运行 tools/make_test_wav.cpp 生成 test.wav\n";
        return 1;
    }

    // M8：带初始化的 if + optional，成功才进分支
    if (auto w = WavReader::load(argv[1]); w) {
        std::cout << *w << "\n";
        std::cout << "峰值振幅  : " << peakAmplitude(w->samples()) << "\n";
        std::cout << "RMS       : " << rms(w->samples()) << "\n";
        return 0;
    }
    std::cerr << "解析失败。\n";
    return 2;
}
