// 练习 4：非类型模板参数 —— 定长数组 Array<T, N>
// 编译：cl /EHsc /std:c++17 /W4 ex4_array.cpp
#include <iostream>

// T 是类型参数，N 是非类型参数（编译期 int 常量）
template <typename T, int N>
class Array {
public:
    // 非 const 版：可读可写
    T& operator[](int i) { return data_[i]; }
    // const 版：const 对象/const 引用也能读
    const T& operator[](int i) const { return data_[i]; }

    int size() const { return N; }        // 大小编译期已知

    void fill(const T& v) {
        for (int i = 0; i < N; ++i) data_[i] = v;
    }
private:
    T data_[N];        // N 编译期确定，栈上定长数组，零堆分配
};

int main() {
    Array<int, 5> a;
    a.fill(7);
    std::cout << "a (size " << a.size() << "): ";
    for (int i = 0; i < a.size(); ++i) std::cout << a[i] << " ";
    std::cout << "\n";

    a[2] = 99;         // 用非 const operator[] 写
    std::cout << "a[2] 改后 = " << a[2] << "\n";

    // 不同的 N 是不同的类型：Array<double,3> 与 Array<int,5> 毫无关系
    Array<double, 3> b;
    b.fill(1.5);
    std::cout << "b (size " << b.size() << "): ";
    for (int i = 0; i < b.size(); ++i) std::cout << b[i] << " ";
    std::cout << "\n";
    return 0;
}
