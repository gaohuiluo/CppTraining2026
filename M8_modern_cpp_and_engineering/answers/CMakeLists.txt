# M8 answers 示例 CMakeLists.txt
# 演示：C++17 标准设定 + 多个可执行目标 + 线程库 + 注册测试
#
# 构建流程：
#   cmake -S . -B build
#   cmake --build build
#   ctest --test-dir build        # 跑注册的测试
#
# Visual Studio：直接「打开文件夹」选中本目录，VS 会自动识别 CMake。

cmake_minimum_required(VERSION 3.15)
project(M8Answers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)            # 用 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # 强制，不允许降级
set(CMAKE_CXX_EXTENSIONS OFF)         # 关掉编译器私有扩展，用标准 C++

# 线程库（thread/mutex 相关目标需要链接它）
find_package(Threads REQUIRED)

# 每个练习一个可执行目标
add_executable(ex1_optional            ex1_optional.cpp)
add_executable(ex2_variant             ex2_variant.cpp)
add_executable(ex3_string_view         ex3_string_view.cpp)
add_executable(ex4_structured_binding  ex4_structured_binding.cpp)
add_executable(ex5_constexpr           ex5_constexpr.cpp)
add_executable(ex6_enum_class          ex6_enum_class.cpp)
add_executable(ex7_exceptions          ex7_exceptions.cpp)
add_executable(ex10_unittest           ex10_unittest.cpp)

# 线程相关目标：链接 Threads::Threads
add_executable(ex8_thread_mutex ex8_thread_mutex.cpp)
add_executable(ex9_atomic       ex9_atomic.cpp)
target_link_libraries(ex8_thread_mutex PRIVATE Threads::Threads)
target_link_libraries(ex9_atomic       PRIVATE Threads::Threads)

# 把单元测试注册进 CTest：退出码非零即算失败
enable_testing()
add_test(NAME ex10_unittest COMMAND ex10_unittest)

# 排错时可给某个目标开 sanitizer（GCC/Clang；MSVC 用 /fsanitize=address）：
#   target_compile_options(ex8_thread_mutex PRIVATE -fsanitize=thread)
#   target_link_options(ex8_thread_mutex    PRIVATE -fsanitize=thread)
