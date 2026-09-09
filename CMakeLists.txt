cmake_minimum_required(VERSION 3.16)
project(mango_net LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译警告，方便提前发现bug
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -g)
endif()

# 内存池静态库
add_library(mango_pool STATIC
    src/mango_pool/mango_pool.cpp
)
target_include_directories(mango_pool PUBLIC src)

# GTest单元测试
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(test_pool tests/test_pool.cpp)
target_link_libraries(test_pool mango_pool ${GTEST_LIBRARIES} pthread)

add_executable(bench_pool bench/bench_pool.cpp)
target_link_libraries(bench_pool mango_pool pthread)

