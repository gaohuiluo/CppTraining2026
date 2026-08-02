// 练习 10：组合实战——学生成绩处理
// 编译：cl /EHsc /std:c++17 /W4 ex10_students.cpp
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>

struct Student {
    std::string name;
    int score;
};

int main() {
    std::vector<Student> students{
        {"Tom", 88}, {"Jerry", 55}, {"Spike", 72}, {"Tyke", 40}, {"Nibbles", 95}
    };

    // 1. 按分数降序
    std::sort(students.begin(), students.end(),
              [](const Student& a, const Student& b) { return a.score > b.score; });
    std::cout << "按分数降序:\n";
    for (const auto& s : students)
        std::cout << "  " << s.name << " " << s.score << '\n';

    // 2. 及格人数
    int pass = std::count_if(students.begin(), students.end(),
                             [](const Student& s) { return s.score >= 60; });
    std::cout << "及格人数: " << pass << '\n';

    // 3. 平均分：accumulate 求总分，再除以人数（用 double 除避免整数截断）
    int total = std::accumulate(students.begin(), students.end(), 0,
                                [](int acc, const Student& s) { return acc + s.score; });
    double avg = static_cast<double>(total) / students.size();
    std::cout << "平均分: " << avg << '\n';

    // 4. 第一个不及格的学生（已按降序排，末尾是低分，find_if 仍能定位第一个满足者）
    if (auto it = std::find_if(students.begin(), students.end(),
                               [](const Student& s) { return s.score < 60; });
        it != students.end())
        std::cout << "第一个不及格: " << it->name << '\n';

    // 5. 最高分学生
    auto top = std::max_element(students.begin(), students.end(),
                                [](const Student& a, const Student& b) { return a.score < b.score; });
    std::cout << "最高分: " << top->name << " " << top->score << '\n';

    // 6. 用 transform 抽出所有名字到新 vector
    std::vector<std::string> names;
    names.reserve(students.size());
    std::transform(students.begin(), students.end(), std::back_inserter(names),
                   [](const Student& s) { return s.name; });
    std::cout << "名单: ";
    for (const auto& n : names) std::cout << n << ' ';
    std::cout << '\n';
}
