// 综合 mini 配套：RAII 封装 FILE*（可读写、禁止拷贝、支持移动）
// 编译：cl /EHsc /std:c++17 /W4 raii_file.cpp
#include <iostream>
#include <string>
#include <cstdio>
#include <utility>   // std::exchange

class TextFile {
public:
    TextFile(const std::string& path, const std::string& mode)
        : fp_(std::fopen(path.c_str(), mode.c_str())) {}

    ~TextFile() { close(); }

    // 禁止拷贝：两个对象持同一个 FILE* 会 double close -> 未定义行为
    TextFile(const TextFile&)            = delete;
    TextFile& operator=(const TextFile&) = delete;

    // 支持移动：把句柄偷过来，对方置空
    TextFile(TextFile&& other) noexcept
        : fp_(std::exchange(other.fp_, nullptr)) {}
    TextFile& operator=(TextFile&& other) noexcept {
        if (this != &other) {
            close();
            fp_ = std::exchange(other.fp_, nullptr);
        }
        return *this;
    }

    bool valid() const { return fp_ != nullptr; }

    void writeLine(const std::string& line) {
        if (fp_) std::fputs((line + "\n").c_str(), fp_);
    }

    // 读出整个文件内容
    std::string readAll() {
        std::string out;
        if (!fp_) return out;
        char buf[256];
        while (std::fgets(buf, sizeof(buf), fp_)) out += buf;
        return out;
    }

private:
    void close() {
        if (fp_) {
            std::fclose(fp_);
            fp_ = nullptr;
        }
    }
    std::FILE* fp_;
};

int main() {
    const std::string path = "m4_mini_tmp.txt";

    {
        TextFile out(path, "w");            // RAII：构造即打开
        if (!out.valid()) { std::cout << "打开失败\n"; return 1; }
        out.writeLine("line 1");
        out.writeLine("line 2");
    }   // out 离开作用域自动 fclose，无需手动关

    {
        TextFile in(path, "r");
        if (!in.valid()) { std::cout << "打开失败\n"; return 1; }
        std::cout << "读回内容:\n" << in.readAll();
    }   // in 自动关闭

    return 0;
}
