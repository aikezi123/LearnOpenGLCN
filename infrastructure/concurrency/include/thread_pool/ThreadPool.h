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

namespace engineeringlab::infrastructure {

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
    // submit()的核心含义是，把一个原本同步返回R的Callable包装成异步任务，由worker执行task，再通过promise保存结果，submit返回异步的future
    // submit的返回类型与最终取出任务执行std::apply()之间的类型保持一致，std::apply的结果会被std::promise保存进共享内存中
    auto submit(F&& task, Args&&... taskArgs)
    ->std::future<std::invoke_result_t<std::decay_t<F>&, std::decay_t<Args>...>> {
        using StoredTaskType = std::decay_t<F>;              // 任务保存类型。
        using StoredArgsTuple = std::tuple<std::decay_t<Args>...>; // 任务参数打包成的Tuple类型
        using ReturnType  = std::invoke_result_t<std::decay_t<F>&, std::decay_t<Args>...>; // 任务返回类型 

        // submit生命周期很短，但是promise需要在任务取出时使用，因此使用shared_ptr把promise打包放进闭包任务中，延长声明周期。
        // 不用std::unique_ptr是因为post用std::function接收闭包任务,std::function必须能拷贝，因此闭包对象必须能拷贝，如果用unique_ptr,则只能移动
        auto promise = std::make_shared<std::promise<ReturnType >>();
        std::future<ReturnType> future = promise->get_future();

        // 将promise和有参数任务及其参数包打包成无参数的闭包Lambda对象。
        // 最后worker线程会取出闭包任务执行，并把结果放入promise里，这样submit可以得到异步结果。
        auto taskFunction = [
            promise,
            storedTask = StoredTaskType(std::forward<F>(task)),
            taskArgTuple = StoredArgsTuple(std::forward<Args>(taskArgs)...)
        ]() mutable {
            try {
                if constexpr(std::is_void_v<ReturnType >) {
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


} // engineeringlab::infrastructure





