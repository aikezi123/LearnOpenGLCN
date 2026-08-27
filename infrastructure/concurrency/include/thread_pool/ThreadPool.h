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

template<typename F, typename... Args>
auto submit(F&& function, Args&&... args)
    -> std::future<std::invoke_result_t<std::decay_t<F>&, std::decay_t<Args>...>>
{
    using StoredFunction = std::decay_t<F>;
    using ArgsTuple = std::make_tuple<std::decay_t<Args>...>;
    using ReturnType = std::invoke_result_t<StoredFunction&, std::decay_t<Args>...>;

    auto promise = std::make_shared<std::promise<ReturnType>>();
    std::future<ReturnType> future = promise->get_future();

    auto invocationTask = [
        promise,
        function = StoredFunction(std::forward<F>(function)),
        argsTuple = ArgsTuple(std::forward<Args>(args)...)
    ]() mutable {
        try {
            auto invokeFunction = [&function](auto&&... unpacked) -> decltype(auto) {
                return std::invoke(function, std::forward<decltype(unpacked)>(unpacked)...);
            };

            if constexpr (std::is_void_v<ReturnType>) {
                std::apply(invokeFunction, std::move(argsTuple));
                promise->set_value();
            } else {
                promise->set_value(std::apply(invokeFunction, std::move(argsTuple)));
            }
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    };

    auto taskHolder = std::make_shared<decltype(invocationTask)>(std::move(invocationTask));

    post([taskHolder]() {
        (*taskHolder)();
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