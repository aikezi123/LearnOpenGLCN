#include <thread_pool/ThreadPool.h>
#include <stdexcept>

namespace learnopengl::infrastructure {

ThreadPool::ThreadPool(size_t count) {
    if (count <= 0) {
        throw std::invalid_argument("线程池初始化失败，线程池中线程数量应大于0");
    }
    m_workers.reserve(count);
    for (int i = 0; i < count; ++i) {
        m_workers.emplace_back(&ThreadPool::workerLoop, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_requireShutdown = true;
    }
    m_condition.notify_all();

    // thread对象不可复制不可移动，因此这里必须加&
    for (auto &worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::workerLoop() {
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            // 当收到请求销毁线程池或任务队列不为空时唤醒线程，取任务出来处理
            m_condition.wait(locker, [this]() {
                return m_requireShutdown || !m_tasks.empty();
            });

            // 请求销毁线程池，并且任务队列为空时，退出循环
            if (m_requireShutdown && m_tasks.empty()) {
                break;
            }

            // 从任务队列中取出一个任务
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        try {
            task();
        } catch (...) {

        }

    }

    // worker线程运行结束
}

void ThreadPool::post(Task task) {
    if (task == nullptr) {
        throw std::runtime_error("不能向线程池投递一个空的任务");
    }

    {
        std::lock_guard<std::mutex> locker(m_mutex);

        // 如果正在销毁线程池，则不允许任何任务进队列
        if (m_requireShutdown) {
            throw std::runtime_error("线程池正在析构，不能投递任务");
        }

        m_tasks.push(std::move(task));
    }
    // 唤醒一个worker线程工作
    m_condition.notify_one();

}



} // learnopengl::infrastructure
