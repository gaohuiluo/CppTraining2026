// 练习 4：手写 RAII 类封装 FILE*
// 编译：cl /EHsc /std:c++17 /W4 ex4_raii_file.cpp
#include <iostream>
#include <cstdio>

class FileGuard {
public:
    // 构造 = 获取资源(fopen)
    FileGuard(const char* path, const char* mode)
        : fp_(std::fopen(path, mode)) {}

    // 析构 = 释放资源(fclose)。无论从哪条路径离开作用域都会执行。
    ~FileGuard() {
        if (fp_) {
            std::fclose(fp_);
            std::cout << "文件已关闭\n";
        }
    }

    bool valid() const { return fp_ != nullptr; }
    std::FILE* get() const { return fp_; }

private:
    std::FILE* fp_;
};

int main() {
    {
        FileGuard f("m4_ex4_tmp.txt", "w");   // 构造：打开文件
        if (!f.valid()) {
            std::cout << "打开失败\n";
            return 1;   // 就算这里提前 return，f 析构也会自动 fclose
        }
        std::fprintf(f.get(), "hello RAII\n");
        std::cout << "写入完成，离开作用域将自动关闭\n";
    }   // f 离开作用域 -> 自动析构 -> 自动 fclose，打印"文件已关闭"

    // 对比 C：
    //   FILE* fp = fopen("x", "w");
    //   if (!fp) return 1;
    //   ... 每一条 return / 错误分支前都得记得 fclose(fp) ...
    //   fclose(fp);
    // RAII 只在析构里写一次 fclose，所有退出路径(含异常)都自动释放。
    return 0;
}
