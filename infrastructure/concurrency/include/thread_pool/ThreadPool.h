#pragma once
#include <thread>
#include <condition_variable>
#include <memory>
#include <future>
#include <queue>
#include <mutex>
#include <functional>
#include <vector>

namespace learnopengl::infrastructure {

class ThreadPool {
public:
    explicit ThreadPool(size_t count);
    ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool& operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool& operator=(ThreadPool &&) = delete;

    using Task = std::function<void()>;
    
    // 投递任务
    void post(Task task);

private:
    // 每个线程都长期运行这个函数
    void workerLoop();

private:
    // 锁保护请求退出变量以及任务队列
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_requireShutdown{false};
    std::queue<Task> m_tasks;
    
    // 线程队列
    std::vector<std::thread> m_workers;

};


} // learnopengl::infrastructure