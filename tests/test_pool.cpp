#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <set>

#include "mango_pool/mango_pool.h"

using namespace mango_pool;

// 测试：单次分配 + 释放
TEST(MangoPoolTest, SimpleAllocAndFree) {
    MangoPool pool(1024 * 1024);  // 1MB
    void* p = pool.allocate(100);
    ASSERT_NE(p, nullptr);

    // 返回的内存应当可写
    std::memset(p, 0xAB, 100);

    pool.deallocate(p);

    // 释放后再次分配，应当能复用这块内存
    void* q = pool.allocate(100);
    ASSERT_NE(q, nullptr);
    pool.deallocate(q);
}

// 测试：多次分配，指针互不重叠
TEST(MangoPoolTest, MultiAlloc) {
    MangoPool pool(1024 * 1024);
    void* p1 = pool.allocate(64);
    void* p2 = pool.allocate(128);
    void* p3 = pool.allocate(256);

    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    ASSERT_NE(p3, nullptr);

    // 三个返回的数据区不能重叠
    auto addr = [](void* p) { return reinterpret_cast<uintptr_t>(p); };
    uintptr_t a1 = addr(p1), a2 = addr(p2), a3 = addr(p3);

    // 用区间判断，避免只比较首地址
    auto overlap = [](uintptr_t x, size_t xs, uintptr_t y, size_t ys) {
        return x < y + ys && y < x + xs;
    };

    EXPECT_FALSE(overlap(a1, 64, a2, 128));
    EXPECT_FALSE(overlap(a1, 64, a3, 256));
    EXPECT_FALSE(overlap(a2, 128, a3, 256));

    // 分别写一下，确保内存真的可用
    std::memset(p1, 0x11, 64);
    std::memset(p2, 0x22, 128);
    std::memset(p3, 0x33, 256);

    pool.deallocate(p1);
    pool.deallocate(p2);
    pool.deallocate(p3);
}

// 测试：申请超出容量，返回空
TEST(MangoPoolTest, AllocOverflow) {
    MangoPool pool(1024);  // 1KB 小池
    void* p = pool.allocate(2048);
    ASSERT_EQ(p, nullptr);

    // 超容量失败后，小分配仍然应该成功
    void* q = pool.allocate(64);
    ASSERT_NE(q, nullptr);
    pool.deallocate(q);
}

// 测试：释放后再分配，可复用空间（不要求返回同一地址）
TEST(MangoPoolTest, ReuseAfterFree) {
    MangoPool pool(4096);

    void* p1 = pool.allocate(100);
    void* p2 = pool.allocate(100);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    EXPECT_NE(p1, p2);

    pool.deallocate(p1);
    pool.deallocate(p2);

    // 池子应当恢复为一块大空闲块，能再分配接近整池大小的内存
    void* big = pool.allocate(4000);
    ASSERT_NE(big, nullptr);
    pool.deallocate(big);
}

// 测试：反复分配/释放不崩溃、不耗尽
TEST(MangoPoolTest, StressSmallAlloc) {
    MangoPool pool(1024 * 1024);

    for (int round = 0; round < 1000; ++round) {
        void* a = pool.allocate(32);
        void* b = pool.allocate(64);
        ASSERT_NE(a, nullptr);
        ASSERT_NE(b, nullptr);

        pool.deallocate(a);
        pool.deallocate(b);
    }
}

// 测试：非法输入
TEST(MangoPoolTest, EdgeCases) {
    MangoPool pool(1024);

    // 分配 0 字节应当返回 nullptr
    EXPECT_EQ(pool.allocate(0), nullptr);

    // 释放 nullptr 应当安全
    pool.deallocate(nullptr);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}