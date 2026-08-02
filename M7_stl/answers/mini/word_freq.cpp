// 综合项目 mini：词频统计器
// 编译：cl /EHsc /std:c++17 /W4 word_freq.cpp
// 思路：unordered_map 计数 -> 倒进 vector<pair> -> sort(多键 lambda) -> 输出 Top N
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <cctype>

// 归一化：转小写 + 去掉首尾标点，让 "The" 和 "the."、"word" 归到一起
static std::string normalize(std::string w) {
    // 去首部标点
    std::size_t b = 0;
    while (b < w.size() && std::ispunct(static_cast<unsigned char>(w[b]))) ++b;
    // 去尾部标点
    std::size_t e = w.size();
    while (e > b && std::ispunct(static_cast<unsigned char>(w[e - 1]))) --e;
    w = w.substr(b, e - b);
    // 转小写：transform 逐字符映射
    std::transform(w.begin(), w.end(), w.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return w;
}

int main() {
    // 内置多行文本当输入（也可换成从 std::cin 读）
    const std::string text =
        "The quick brown fox jumps over the lazy dog.\n"
        "The dog barks, and the FOX runs. The quick fox!\n"
        "A dog is a dog, but the fox is quick, quick, quick.\n";

    const std::size_t topN = 5;

    // 1+2. 按空白切词，unordered_map 计数（哈希表 O(1) 累加）
    std::unordered_map<std::string, int> freq;
    std::istringstream iss(text);
    std::string token;
    while (iss >> token) {
        std::string w = normalize(token);
        if (!w.empty())
            ++freq[w];            // 不存在则默认插 0，再 +1
    }

    // 3. 倒进 vector<pair> 以便排序（map/unordered_map 本身不能按值排序）
    std::vector<std::pair<std::string, int>> items(freq.begin(), freq.end());

    // 4. 主键：频率降序；次键：频率相同时按单词字典序升序
    std::sort(items.begin(), items.end(),
              [](const auto& a, const auto& b) {
                  if (a.second != b.second) return a.second > b.second;  // 频率高的在前
                  return a.first < b.first;                              // 平局按字典序
              });

    // 5. 统计信息：总词数（频率之和）与不同单词数
    int totalWords = std::accumulate(items.begin(), items.end(), 0,
                                     [](int acc, const auto& p) { return acc + p.second; });
    std::cout << "总词数: " << totalWords << "，不同单词数: " << items.size() << '\n';

    // 6. 输出 Top N（不足 N 就全出）
    std::cout << "Top " << topN << ":\n";
    std::size_t limit = std::min(topN, items.size());
    for (std::size_t i = 0; i < limit; ++i)
        std::cout << "  " << (i + 1) << ". " << items[i].first
                  << " (" << items[i].second << ")\n";
}
