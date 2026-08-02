# mini：生产者-消费者 队列 的最小 CMakeLists.txt
#
# 构建 + 测试：
#   cmake -S . -B build
#   cmake --build build
#   ctest --test-dir build
#
# Visual Studio：「打开文件夹」选中本目录即可，VS 原生识别 CMake。

cmake_minimum_required(VERSION 3.15)
project(ProducerConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)          # 线程库

# demo：生产者-消费者演示程序
add_executable(pc_demo main.cpp)
target_link_libraries(pc_demo PRIVATE Threads::Threads)

# tests：零依赖断言测试
add_executable(pc_tests tests.cpp)
target_link_libraries(pc_tests PRIVATE Threads::Threads)

# 注册到 CTest：pc_tests 退出码非零即判失败
enable_testing()
add_test(NAME queue_tests COMMAND pc_tests)

# 排错建议：查并发 bug 时给目标开 ThreadSanitizer（GCC/Clang）
#   target_compile_options(pc_demo PRIVATE -fsanitize=thread -g)
#   target_link_options(pc_demo    PRIVATE -fsanitize=thread)
