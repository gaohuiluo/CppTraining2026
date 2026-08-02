// 模板化小端字节读取（用到 M6：模板 + if constexpr + static_assert）
#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <stdexcept>

// 从 data[offset] 起，按小端(little-endian)读取 T 类型无符号整数。
// 逐字节移位组装，不依赖运行平台的字节序 —— 这是解析二进制格式的通用手法。
template <typename T>
T readLE(const std::vector<uint8_t>& data, std::size_t offset) {
    static_assert(std::is_unsigned<T>::value,
                  "readLE 只支持无符号整数类型 (uint16_t/uint32_t 等)");
    if (offset + sizeof(T) > data.size())
        throw std::out_of_range("readLE: 读取越界");

    T value = 0;
    // 小端：低地址存低位字节，所以第 i 个字节左移 8*i 位
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        value |= static_cast<T>(data[offset + i]) << (8 * i);
    }
    return value;
}

// 读取 4 字节的块标识（如 "RIFF"/"fmt "），返回 std::string 便于比较
inline std::string readTag(const std::vector<uint8_t>& data, std::size_t offset) {
    if (offset + 4 > data.size())
        throw std::out_of_range("readTag: 读取越界");
    return std::string(data.begin() + offset, data.begin() + offset + 4);
}
