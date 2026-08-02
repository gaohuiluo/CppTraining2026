// 练习 3：四类经典内存错误（反面教材）+ 正确写法
// 编译：cl /EHsc /std:c++17 /W4 ex3_memory_bugs.cpp
#include <iostream>

int main() {
    // ---------- 1. 内存泄漏 ----------
    // 【错误】
    //   int* p = new int(42);
    //   // ... 用完忘了 delete ... 函数结束，p(栈指针)没了，堆内存再没人能释放 -> 泄漏
    // 【正确】用完配对 delete；或者干脆用 unique_ptr 自动管理。
    {
        int* p = new int(42);
        std::cout << "1. leak-fixed: " << *p << "\n";
        delete p;   // 配对释放
    }

    // ---------- 2. 悬垂指针 ----------
    // 【错误】
    //   int* p = new int(42);
    //   delete p;
    //   std::cout << *p;   // p 指向已释放内存 -> 悬垂，未定义行为
    // 【正确】delete 后立刻置空，并在使用前判空。
    {
        int* p = new int(42);
        delete p;
        p = nullptr;        // 置空，避免误用悬垂指针
        if (p) std::cout << *p;
        std::cout << "2. dangling-fixed: p 已置空\n";
    }

    // ---------- 3. double free ----------
    // 【错误】
    //   int* p = new int(42);
    //   delete p;
    //   delete p;   // 同一块内存删两次 -> 破坏堆结构，通常崩溃
    // 【正确】只 delete 一次；delete 后置空(对空指针 delete 是安全的无操作)。
    {
        int* p = new int(42);
        delete p;
        p = nullptr;
        delete p;   // delete nullptr 安全，什么都不做
        std::cout << "3. double-free-fixed: 只真正删一次\n";
    }

    // ---------- 4. new[] / delete 不匹配 ----------
    // 【错误】
    //   int* arr = new int[100];
    //   delete arr;    // 应该 delete[]，用 delete 是未定义行为
    // 【正确】new[] 必须配 delete[]。
    {
        int* arr = new int[100];
        arr[0] = 7;
        std::cout << "4. array-fixed: arr[0]=" << arr[0] << "\n";
        delete[] arr;   // 配对的数组版本
    }

    return 0;
}
