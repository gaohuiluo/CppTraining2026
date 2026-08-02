# WAV 解析器 CMake 构建（M8 学过）
# 用法：
#   cmake -B build -S .
#   cmake --build build
cmake_minimum_required(VERSION 3.15)
project(wav_parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(wav_parser main.cpp wav_reader.cpp)

# 生成测试 WAV 的小工具
add_executable(make_test_wav ../tools/make_test_wav.cpp)
