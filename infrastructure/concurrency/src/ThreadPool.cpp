#include <thread_pool/ThreadPool.h>
#include <stdexcept>


namespace learnopengl::infrastructure {

ThreadPool::ThreadPool(std::size_t count) {

    if (count == 0) {
        throw std::invalid_argument("线程池初始化失败,创建线程数量需大于0");
    }

    m_workers.reserve(count);

    try {
        for (std::size_t i = 0; i < count; ++i) {
            m_workers.emplace_back(&ThreadPool::workerLoop, this);
        }
    } catch(...) {
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            m_requireShutdown = true;
        }
        m_cv.notify_all();

        for (auto &worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        throw;
    }

}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> locker(m_shutdownMutex);
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            m_requireShutdown = true;
        }

        m_cv.notify_all();

        for (auto &worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

}

void ThreadPool::workerLoop() {
    while (true)  {
        Task task;
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            m_cv.wait(locker, [this]() {
                return m_requireShutdown || !m_tasks.empty();
            });

            if (m_requireShutdown && m_tasks.empty()) {
                break;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        task();

    }

}

void ThreadPool::post(Task task) {
    if (task == nullptr) {
        throw std::invalid_argument("线程池投递任务失败");
    }
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        
        if (m_requireShutdown) {
            return;
        }

        m_tasks.push(std::move(task));
    }
    m_cv.notify_one();
}


} // learnopengl::infrastructure