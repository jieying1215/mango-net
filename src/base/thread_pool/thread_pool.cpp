#include "base/thread_pool/thread_pool.h"

namespace mango_pool {

ThreadPool::ThreadPool(size_t threads) : stop_(false) {
    if (threads == 0) {
        threads = 1;
    }
    workers_.reserve(threads);

    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this]() {
            for (;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cv_.wait(lock, [this]() {
                        return stop_ || !tasks_.empty();
                    });

                    if (stop_ && tasks_.empty()) {
                        return;
                    }

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();

    for (auto& t : workers_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

size_t ThreadPool::pending_tasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

} // namespace mango_pool