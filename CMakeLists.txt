cmake_minimum_required(VERSION 3.16)
project(mango_net LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 只对 C++ 目标加，不污染全局
add_library(mango_pool STATIC
    src/mango_pool/mango_pool.cpp
)
target_include_directories(mango_pool PUBLIC src)
target_compile_options(mango_pool PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# 测试
enable_testing()
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(test_pool tests/test_pool.cpp)
target_link_libraries(test_pool PRIVATE
    mango_pool
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

include(GoogleTest)
gtest_discover_tests(test_pool)