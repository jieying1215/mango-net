cmake_minimum_required(VERSION 3.16)
project(mango_net LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------- 基础库：内存池 + 线程池 ----------
add_library(mango_pool STATIC
    src/mango_pool/mango_pool.cpp
    src/base/thread_pool/thread_pool.cpp
)
target_include_directories(mango_pool PUBLIC src)

find_package(Threads REQUIRED)
target_link_libraries(mango_pool PUBLIC Threads::Threads)

target_compile_options(mango_pool PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# ---------- 测试 ----------
enable_testing()
find_package(GTest REQUIRED)
include(GoogleTest)          # ← 提供 gtest_discover_tests，必须在使用前 include

# 内存池测试
add_executable(test_pool tests/test_pool.cpp)
target_link_libraries(test_pool PRIVATE
    mango_pool
    GTest::gtest
    Threads::Threads
)
gtest_discover_tests(test_pool)

# 线程池测试
add_executable(test_thread_pool tests/test_thread_pool.cpp)
target_link_libraries(test_thread_pool PRIVATE
    mango_pool
    GTest::gtest
    Threads::Threads
)
gtest_discover_tests(test_thread_pool)