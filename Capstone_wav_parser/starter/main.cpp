// WAV 解析器 主程序骨架
// 编译：cl /EHsc /std:c++17 /W4 main.cpp wav_reader.cpp
#include "wav_reader.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "用法: " << argv[0] << " <文件.wav>\n";
        return 1;
    }

    // TODO(阶段5): 调 WavReader::load，用 if(auto w = ...; w) 处理 optional，
    //              成功打印 *w、峰值、RMS；失败报错。
    (void)argc;
    return 0;
}
