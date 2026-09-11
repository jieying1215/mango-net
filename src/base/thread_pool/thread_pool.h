#ifndef MANGO_THREAD_POOL_H
#define MANGO_THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace mango_pool {

class ThreadPool {
public:
    // 构造：启动 threads 个工作线程
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());

    // 析构：优雅停止，等待所有任务完成
    ~ThreadPool();

    // 提交任务，返回 future 以获取返回值
    // F 可调用对象，Args 参数列表
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    // 禁止拷贝
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 统计
    size_t thread_count() const { return workers_.size(); }
    size_t pending_tasks() const;

private:
    std::vector<std::thread> workers_;               // 工作线程
    std::queue<std::function<void()>> tasks_;        // 任务队列

    mutable std::mutex mutex_;                       // 保护 tasks_
    std::condition_variable cv_;                     // 任务到达/停止通知
    bool stop_;                                      // 停止标志
};

// 模板实现放在头文件里
template <typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
    using ReturnType = typename std::invoke_result<F, Args...>::type;

    // 把任务和参数打包成 shared_ptr<packaged_task>
    // 用 shared_ptr 是因为 std::function 要求可拷贝，而 packaged_task 不可拷贝
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<ReturnType> result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_) {
            throw std::runtime_error("ThreadPool::submit on stopped pool");
        }
        // 用 lambda 包一层，把 packaged_task 转成 void() 放进队列
        tasks_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return result;
}

} // namespace mango_pool

#endif // MANGO_THREAD_POOL_H