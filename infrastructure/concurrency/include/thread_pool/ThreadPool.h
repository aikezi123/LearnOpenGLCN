#pragma once
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>
#include <type_traits>
#include <vector>

namespace learnopengl::infrastructure {

class ThreadPool {
public:
    using Task = std::function<void()>;

public:
    explicit ThreadPool(std::size_t count);
    ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool& operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool& operator=(ThreadPool &&) = delete;

    // 投递无需返回结果的任务
    void post(Task task);

    // 投递需要返回结果的任务
    // 当前阶段
    // 1. 支持任意非void返回类型
    // 2. 暂时只支持无参数Callable
    // 3. 暂时按值接收Callable，后续在学习F&&和std::forward
    template<typename F>
    std::future<std::invoke_result_t<F>> submit(F task) {
        using ReturnType = std::invoke_result_t<F>;

        // 当前阶段暂不处理void返回值
        static_assert(!std::is_void_v<ReturnType>, "当前阶段submit暂时不支持void返回类型");

        // promise负责保存wokrer线程最终产生的结果或异常
        auto promise = std::make_shared<std::promise<ReturnType>>();
        std::future<std::invoke_result_t<F>> future = promise->get_future();

        post([promise, task = std::move(task)]() mutable {
            try {
                std::invoke_result_t<F> result = task();
                promise->set_value(std::move(result));
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });

        return future;
        
    }

    void shutdown();

private:
    void workerLoop();

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<Task> m_tasks;
    bool m_requireShutdown{false};

    std::mutex m_shutdownMutex;
    std::vector<std::thread> m_workers;


};


} // learnopengl::infrastructure