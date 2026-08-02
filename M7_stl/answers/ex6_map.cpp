// 练习 6：map 与 unordered_map
// 编译：cl /EHsc /std:c++17 /W4 ex6_map.cpp
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>

int main() {
    std::map<std::string, int> age;
    age["Tom"] = 20;                 // operator[] 不存在则插入、存在则赋值
    age["Jerry"] = 18;
    age["Spike"] = 25;

    std::cout << "map 遍历（按 key 有序）:\n";
    for (const auto& [name, a] : age)         // C++17 结构化绑定拆 pair
        std::cout << "  " << name << " -> " << a << '\n';   // Jerry/Spike/Tom 字典序

    // map[] 的坑：查一个不存在的 key，会顺手默认构造一个插进去
    std::cout << "查询前 size=" << age.size() << '\n';
    int x = age["Ghost"];            // "Ghost" 不存在 -> 被插入，值默认为 0
    std::cout << "age[\"Ghost\"]=" << x << "，查询后 size=" << age.size()
              << "（被顺手插入了）\n";

    // 只读查询用 find，不会插入
    if (age.find("Nobody") == age.end())
        std::cout << "find(\"Nobody\") 没找到，且 size 仍是 " << age.size() << '\n';

    // 同样的数据放进 unordered_map：遍历顺序不保证（哈希分布决定，非插入序/key 序）
    std::unordered_map<std::string, int> uage{{"Tom", 20}, {"Jerry", 18}, {"Spike", 25}};
    std::cout << "unordered_map 遍历（顺序无意义）:\n";
    for (const auto& [name, a] : uage)
        std::cout << "  " << name << " -> " << a << '\n';

    // 选择：需要有序遍历/范围查询用 map；只要快速按 key 查存、不关心顺序用 unordered_map。
}
