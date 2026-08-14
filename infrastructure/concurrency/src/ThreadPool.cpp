#include <thread_pool/ThreadPool.h>
#include <stdexcept>

namespace learnopengl::infrastructure {

ThreadPool::ThreadPool(size_t count) {
    if (count <= 0) {
        throw std::invalid_argument("线程池初始化失败，线程池中线程数量应大于0");
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
        m_condition.notify_all();
        for (auto &worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        // 异常继续向上层传递
        throw;
    }

}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> shutdownlocker(m_shutdownMutex);
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            m_requireShutdown = true;
        } // locker
        m_condition.notify_all();


        // thread对象不可复制可以移动，因此这里必须加&
        for (auto &worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    } // shutdownlocker

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

std::future<int> ThreadPool::submit(std::function<int()> task) {
    if (!task) {
        throw std::runtime_error("不能向线程池投递一个空的任务");
    }

// 这里使用shared_ptr主要有两个原因：
//
// 1. promise必须存活到Worker执行set_value/set_exception。
//    如果捕获局部promise的引用，submit返回后promise已经析构，
//    Worker之后访问该引用会产生悬空引用，属于未定义行为。
//
// 2. std::promise本身不可复制。
//    如果把promise move捕获进lambda，该lambda会成为move-only对象；
//    而当前Task使用std::function<void()>，要求保存的Callable可复制。
//    因此使用可复制的shared_ptr间接管理唯一的promise对象。
    auto promise = std::make_shared<std::promise<int>>();
    std::future<int> future = promise->get_future();

    post([task = std::move(task), promise]() {
        try {
            int result = task();
            promise->set_value(result);
        } catch(...) {
            promise->set_exception(std::current_exception());
        }
    });

    return future;
    // auto packagedTask = std::make_shared<std::packaged_task<int()>>(std::move(task));
    // std::future<int> future = packagedTask->get_future();
    // post([packagedTask]() {
    //     (*packagedTask)();
    // });

}



} // learnopengl::infrastructure
