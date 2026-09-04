#include <thread_pool/ThreadPool.h>
#include <stdexcept>

namespace engineeringlab::infrastructure {

ThreadPool::ThreadPool(std::size_t count) {
    if (count == 0) {
        throw std::invalid_argument("线程池初始化失败，线程数量应大于0");
    }

    m_workers.reserve(count);
    try {
        for (std::size_t i = 0; i < count; ++i) {
            m_workers.emplace_back(&ThreadPool::workerLoop, this);
        }
    } catch (...) {
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            m_requestShutdown = true;
        }
        m_condition.notify_all();

        for (auto &worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        throw std::runtime_error("线程池初始化失败，线程创建失败");
    }

}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> locker(m_shutdownMutex);
        
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            m_requestShutdown = true;
        } // 对m_requestShutdown加锁
        m_condition.notify_all();

        for (auto &worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

    } // 对整个shutdown函数加锁

}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::post(Task task) {
    if (task == nullptr) {
        throw std::invalid_argument("线程池任务投递失败，任务不能为空");
    }

    {
        std::lock_guard<std::mutex> locker(m_mutex);
        if (m_requestShutdown) {
            throw std::runtime_error("线程已退出，不能再投递任务");
        }
        m_tasks.push(std::move(task));
    }
    m_condition.notify_one();
}

void ThreadPool::workerLoop() {
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            // 请求退出或者任务队列不为空时唤醒线程尝试加锁判断条件是否成立
            m_condition.wait(locker, [this]() {
                return m_requestShutdown || !m_tasks.empty();
            });
            
            // 请求退出并且任务处理完了退出循环,线程结束
            if (m_requestShutdown && m_tasks.empty()) {
                break;
            }

            // 取出一个任务开始处理
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        try {
            task();
        } catch(...) {
            // 待处理
        }
    }
}



} // engineeringlab::infrastructure

