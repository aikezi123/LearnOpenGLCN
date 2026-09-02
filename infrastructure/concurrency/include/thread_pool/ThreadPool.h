#pragma once
#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <vector>
#include <type_traits>
#include <memory>
#include <future>
#include <tuple>
#include <cstddef>
#include <utility>

namespace learnopengl::infrastructure {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t count);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    

    using Task = std::function<void()>;
    void shutdown();
    void post(Task task);

    template<typename F, typename... Args>
    auto submit(F&& task, Args&&... taskArgs)->std::future<std::invoke_result_t<std::decay_t<F>&, std::decay_t<Args>...>> {
        using StoredTaskType = std::decay_t<F>;              // 任务保存类型。
        using ArgsTuple = std::tuple<std::decay_t<Args>...>; // 任务参数打包成的Tuple类型
        using TaskReturnType = std::invoke_result_t<std::decay_t<F>&, std::decay_t<Args>...>; // 任务返回类型 

        auto promise = std::make_shared<std::promise<TaskReturnType>>();
        std::future<TaskReturnType> future = promise->get_future();

        // 将有参数任务及其参数包打包成无参数的闭包Lambda对象。
        auto taskFunction = [
            promise,
            storedTask = StoredTaskType(std::forward<F>(task)),
            taskArgTuple = ArgsTuple(std::forward<Args>(taskArgs)...)
        ]() mutable {
            try {
                if constexpr(std::is_void_v<TaskReturnType>) {
                    std::apply(storedTask, std::move(taskArgTuple));
                    promise->set_value();
                } else {
                    promise->set_value(std::apply(storedTask, std::move(taskArgTuple)));
                }
            } catch(...) {
                promise->set_exception(std::current_exception());
            }

        };

        auto taskHold = std::make_shared<decltype(taskFunction)>(std::move(taskFunction));
        post([taskHold]() {
            (*taskHold)();
        });
        
        return future;
    }

private:
    void workerLoop();

private:

    // 保护请求退出状态以及任务队列
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_requestShutdown{false};
    std::queue<Task> m_tasks;

    // 保护shutdown调用函数
    std::mutex m_shutdownMutex;
    std::vector<std::thread> m_workers;

};


} // learnopengl::infrastructure





