#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include<string>
#include<thread>
#include <vector>

#include "base/thread_pool/thread_pool.h"

using namespace mango_pool;

// 测试：基本提交 + future 拿返回值
TEST(ThreadPoolTest, SubmitAndGetResult) {
    ThreadPool pool(4);

    auto f = pool.submit([](int a, int b) { return a + b; }, 3, 4);
    EXPECT_EQ(f.get(), 7);

    auto g = pool.submit([]() { return std::string("hello"); });
    EXPECT_EQ(g.get(), "hello");
}

// 测试：并发执行多个任务
TEST(ThreadPoolTest, ManyTasks) {
    ThreadPool pool(4);

    constexpr int kN = 1000;
    std::vector<std::future<int>> futures;
    futures.reserve(kN);

    for (int i = 0; i < kN; ++i) {
        futures.emplace_back(pool.submit([i]() { return i * i; }));
    }

    long long sum = 0;
    for (int i = 0; i < kN; ++i) {
        sum += futures[i].get();
    }

    // sum(i*i, i=0..999) = 999*1000*1999/6
    long long expected = static_cast<long long>(kN - 1) * kN * (2 * kN - 1) / 6;
    EXPECT_EQ(sum, expected);
}

// 测试：任务真的并发执行
TEST(ThreadPoolTest, TrulyConcurrent) {
    ThreadPool pool(4);

    std::atomic<int> running{0};
    std::atomic<int> max_running{0};

    auto task = [&]() {
        int cur = running.fetch_add(1) + 1;
        int prev = max_running.load();
        while (cur > prev && !max_running.compare_exchange_weak(prev, cur)) {}
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        running.fetch_sub(1);
    };

    std::vector<std::future<void>> fs;
    for (int i = 0; i < 8; ++i) {
        fs.emplace_back(pool.submit(task));
    }
    for (auto& f : fs) f.get();

    // 4 个线程，同时运行数应该 >= 2（放宽断言，避免调度抖动）
    EXPECT_GE(max_running.load(), 2);
}

TEST(ThreadPoolTest, VoidTask) {
    std::atomic<int> counter{0};
    {
        ThreadPool pool(2);
        for (int i = 0; i < 100; ++i) {
            pool.submit([&counter]() { counter.fetch_add(1); });
        }
    }  // pool 析构，等待所有任务完成
    EXPECT_EQ(counter.load(), 100);
}
int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}