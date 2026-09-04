#include <thread_pool/ThreadPool.h>

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace engineeringlab::infrastructure {
namespace {

using namespace std::chrono_literals;

// 所有异步等待都设置上限，避免实现出现死锁时测试进程永久挂起。
constexpr std::chrono::seconds kTaskTimeout = 5s;

// ThreadPool 内部持有线程、互斥量等资源，不应被复制或移动。
// static_assert(condition):编译器断言，结果是true正常编译；结果是false编译报错。
static_assert(!std::is_copy_constructible_v<ThreadPool>);   // T能不能拷贝构造
static_assert(!std::is_copy_assignable_v<ThreadPool>);      // T能不能拷贝复制
static_assert(!std::is_move_constructible_v<ThreadPool>);   // T能不能移动构造
static_assert(!std::is_move_assignable_v<ThreadPool>);      // T能不能移动赋值

// —————————————— EXCEPT_XXX和ASSERT_XXX________________
// EXCEPT_XXX:失败后，当前测试继续执行
// ASSERT_XXX:失败后，当前测试立即结束



// 验证构造参数的下边界：线程池至少需要一个工作线程。
// 向gTest注册一个名为ThreadPoolTest.ConstructionRejectsZeroWorkers的测试用例，运行测试时会产生类似下面的结果
//[ RUN      ] ThreadPoolTest.ConstructionRejectsZeroWorkers
//[       OK ] ThreadPoolTest.ConstructionRejectsZeroWorkers
TEST(ThreadPoolTest, ConstructionRejectsZeroWorkers)
{
    // 期待抛出某一种类型的异常，如果没有抛出异常失败；抛出不是std::invalid_argument的异常也失败
    EXPECT_THROW(ThreadPool pool(0), std::invalid_argument);
}

// 验证 post() 投递的无返回值任务会被工作线程执行，并能在超时前完成。
TEST(ThreadPoolTest, PostExecutesTask)
{
    ThreadPool pool(1);

    // promise/future 用来等待后台任务完成，避免使用不稳定的 sleep。
    std::promise<void> completedPromise;
    std::future<void> completedFuture = completedPromise.get_future();

    pool.post([&completedPromise]() {
        completedPromise.set_value();
    });

    // EXCEPT_EQ:判断两者是否相等==。
    // wait_for()返回std::future_status
    EXPECT_EQ(completedFuture.wait_for(kTaskTimeout), std::future_status::ready);
}

// 验证单个 post() 任务只执行一次，不会因为存在多个工作线程而重复执行。
TEST(ThreadPoolTest, PostedTaskExecutesExactlyOnce)
{
    ThreadPool pool(2);
    std::atomic<int> executionCount{0};

    pool.post([&executionCount]() {
        executionCount.fetch_add(1, std::memory_order_relaxed);
    });

    // shutdown() 返回时，已经接收的任务应全部执行完毕。
    pool.shutdown();

    EXPECT_EQ(executionCount.load(std::memory_order_relaxed), 1);
}

// 验证 post() 任务抛出异常后，异常被线程池隔离，工作线程仍能继续执行后续任务。
TEST(ThreadPoolTest, ThrowingPostedTaskDoesNotStopWorker)
{
    using namespace std::chrono_literals;

    std::promise<int> promise;
    std::future<int> future = promise.get_future();

    ThreadPool pool(1);

    pool.post([]() {
        throw std::runtime_error("异常测试");
    });

    pool.post([&promise]() {
        promise.set_value(2);
    });

    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);

    EXPECT_EQ(future.get(), 2);


}

// 验证 submit() 返回的 future 能取得任务计算结果。
TEST(ThreadPoolTest, SubmitReturnsTaskResult)
{
    ThreadPool pool(1);

    std::future<int> resultFuture = pool.submit([]() {
        return 42;
    });

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_EQ(resultFuture.get(), 42);
}

// 验证 submit() 能保存并转发多个不同类型的任务参数。
TEST(ThreadPoolTest, SubmitPassesMultipleArguments)
{
    ThreadPool pool(1);

    std::future<std::string> resultFuture = pool.submit(
        [](int result, std::string prex) {
            return prex + std::to_string(result);
        }, 
        42, "result="
    );
    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_EQ(resultFuture.get(), "result=42");
}

// 验证返回 void 的任务也能通过 future<void> 报告完成状态。
TEST(ThreadPoolTest, SubmitSupportsVoidResult)
{
    ThreadPool pool(1);
    std::atomic<int> result{0};
    std::future<void> future = pool.submit(
        [&result](int lhs, int rhs) {
            result.store(lhs + rhs);
        },
        22, 20
    );
    ASSERT_EQ(future.wait_for(5s), std::future_status::ready);
    EXPECT_NO_THROW(future.get());
    EXPECT_EQ(result.load(), 42);
}

// 验证 submit() 支持只能移动、不能复制的可调用对象。
TEST(ThreadPoolTest, SubmitSupportsMoveOnlyCallable)
{
    ThreadPool pool(1);

    // 捕获 unique_ptr 后的 lambda 不可复制，用于验证 submit() 支持移动任务。
    std::future<int> resultFuture = pool.submit(
        [value = std::make_unique<int>(42)]() {
            return *value;
        });

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_EQ(resultFuture.get(), 42);
}

// 验证 submit() 支持通过参数包传入只能移动、不能复制的参数。
TEST(ThreadPoolTest, SubmitSupportsMoveOnlyArgument)
{
    ThreadPool pool(1);

    // unique_ptr 只能移动，验证参数包不会要求所有参数都可复制。
    std::future resultFuture = pool.submit(
        [](std::unique_ptr<int> value) {
            return *value;
        },
        std::make_unique<int>(42)
    );

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_EQ(resultFuture.get(), 42);
}

// 验证 submit() 任务只执行一次，同时 future 返回对应的唯一执行结果。
TEST(ThreadPoolTest, SubmittedTaskExecutesExactlyOnce)
{
    ThreadPool pool(2);
    std::atomic<int> executionCount{0};

    std::future<int> resultFuture = pool.submit([&executionCount]() {
        executionCount.fetch_add(1, std::memory_order_relaxed);
        return 42;
    });

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_EQ(resultFuture.get(), 42);

    pool.shutdown();

    EXPECT_EQ(executionCount.load(std::memory_order_relaxed), 1);
}

// 验证 submit() 任务抛出的异常会保存在 future 中，并由 future::get() 原样重新抛出。
TEST(ThreadPoolTest, SubmitPropagatesTaskExceptionThroughFuture)
{
    ThreadPool pool(1);

    std::future<int> resultFuture = pool.submit([]() -> int {
        throw std::runtime_error("task failed");
    });

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);

    // submit() 不应在工作线程吞掉异常，而应由 future::get() 重新抛出。
    try {
        static_cast<void>(resultFuture.get());
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& exception) {
        EXPECT_STREQ(exception.what(), "task failed");  // 专门比较两个c风格字符串const char*内容是否相同
    } catch (...) {
        FAIL() << "Expected std::runtime_error";  // 直接把当前测试标记为失败
    }
}

// 验证 post() 会立即拒绝空的 std::function 任务，并抛出 invalid_argument。
TEST(ThreadPoolTest, EmptyPostTaskIsRejected)
{
    ThreadPool pool(1);
    ThreadPool::Task emptyTask;

    EXPECT_THROW(pool.post(std::move(emptyTask)), std::invalid_argument);
}

// 验证空 submit() 任务的 bad_function_call 会通过 future 传播，而不是逃出工作线程。
TEST(ThreadPoolTest, EmptySubmitTaskPropagatesBadFunctionCallThroughFuture)
{
    ThreadPool pool(1);
    std::function<int()> emptyTask;

    // submit() 本身成功投递包装任务；空函数调用产生的异常保存在 future 中。
    std::future<int> resultFuture = pool.submit(std::move(emptyTask));

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_THROW(static_cast<void>(resultFuture.get()), std::bad_function_call);
}

// 验证 shutdown() 采用排空关闭语义：返回前完成所有已经接收的任务。
TEST(ThreadPoolTest, ShutdownDrainsAcceptedTasks)
{
    ThreadPool pool(2);
    constexpr int kTaskCount = 32;
    std::atomic<int> completedTaskCount{0};

    for (int index = 0; index < kTaskCount; ++index) {
        pool.post([&completedTaskCount]() {
            completedTaskCount.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // 线程池采用排空关闭语义：shutdown() 不丢弃队列中的任务。
    pool.shutdown();

    EXPECT_EQ(completedTaskCount.load(std::memory_order_relaxed), kTaskCount);
}

// 验证未显式调用 shutdown() 时，析构函数仍会排空并完成已经接收的任务。
TEST(ThreadPoolTest, DestructorDrainsAcceptedTasks)
{
    constexpr int kTaskCount = 32;
    std::atomic<int> completedTaskCount{0};

    {
        ThreadPool pool(2);

        for (int index = 0; index < kTaskCount; ++index) {
            pool.post([&completedTaskCount]() {
                completedTaskCount.fetch_add(1, std::memory_order_relaxed);
            });
        }
    }

    // 离开作用域会调用析构函数，析构函数内部应完成排空关闭。
    EXPECT_EQ(completedTaskCount.load(std::memory_order_relaxed), kTaskCount);
}

// 验证 shutdown() 具有幂等性，连续调用多次不会抛出异常或重复关闭出错。
TEST(ThreadPoolTest, ShutdownCanBeCalledMoreThanOnce)
{
    ThreadPool pool(1);

    EXPECT_NO_THROW(pool.shutdown());
    EXPECT_NO_THROW(pool.shutdown());
}

// 验证多个调用线程同时执行 shutdown() 时能安全串行化并正常返回。
TEST(ThreadPoolTest, ConcurrentShutdownCallsAreSafe)
{
    ThreadPool pool(2);
    // 共享启动信号让两个异步调用尽量同时进入 shutdown()。
    std::promise<void> startPromise;
    std::shared_future<void> startFuture = startPromise.get_future().share();

    std::future<void> firstShutdown = std::async(
        std::launch::async,
        [&pool, startFuture]() {
            startFuture.wait();
            pool.shutdown();
        });
    std::future<void> secondShutdown = std::async(
        std::launch::async,
        [&pool, startFuture]() {
            startFuture.wait();
            pool.shutdown();
        });

    startPromise.set_value();

    // 两次调用都必须在超时前完成，且不能向调用方传播异常。
    ASSERT_EQ(firstShutdown.wait_for(kTaskTimeout), std::future_status::ready);
    ASSERT_EQ(secondShutdown.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_NO_THROW(firstShutdown.get());
    EXPECT_NO_THROW(secondShutdown.get());
}

// 验证 shutdown() 完成后，post() 和 submit() 都拒绝接收新任务。
TEST(ThreadPoolTest, ShutdownRejectsNewTasks)
{
    ThreadPool pool(1);
    pool.shutdown();

    // 关闭完成后不再接受 fire-and-forget 或带返回值任务。
    EXPECT_THROW(pool.post([]() {}), std::runtime_error);
    EXPECT_THROW(pool.submit([]() { return 1; }), std::runtime_error);
}

// 验证多个生产者并发投递时，所有任务都不丢失、不重复，并且恰好执行一次。
TEST(ThreadPoolTest, MultipleProducersExecuteEveryTaskExactlyOnce)
{
    constexpr int kProducerCount = 4;
    constexpr int kTasksPerProducer = 100;
    constexpr int kTotalTaskCount = kProducerCount * kTasksPerProducer;

    ThreadPool pool(4);
    // 每个任务拥有独立计数器，可以同时发现任务丢失和重复执行。
    std::array<std::atomic<int>, kTotalTaskCount> executionCounts;

    for (std::atomic<int>& count : executionCounts) {
        count.store(0, std::memory_order_relaxed);
    }

    std::vector<std::thread> producers;
    producers.reserve(kProducerCount);

    // 多个生产者线程同时调用 post()，验证任务队列的入队同步。
    for (int producerIndex = 0; producerIndex < kProducerCount; ++producerIndex) {
        producers.emplace_back(
            [&pool, &executionCounts, producerIndex, kTasksPerProducer]() {
                for (int taskIndex = 0; taskIndex < kTasksPerProducer; ++taskIndex) {
                    const int globalTaskIndex =
                        producerIndex * kTasksPerProducer + taskIndex;

                    pool.post([&executionCounts, globalTaskIndex]() {
                        executionCounts[globalTaskIndex].fetch_add(
                            1,
                            std::memory_order_relaxed);
                    });
                }
            });
    }

    for (std::thread& producer : producers) {
        producer.join();
    }

    // 所有生产者结束后关闭线程池，等待消费者排空队列。
    pool.shutdown();

    for (int taskIndex = 0; taskIndex < kTotalTaskCount; ++taskIndex) {
        // 失败时附带任务编号，便于定位丢失或重复执行的任务。
        SCOPED_TRACE("taskIndex=" + std::to_string(taskIndex));
        EXPECT_EQ(
            executionCounts[taskIndex].load(std::memory_order_relaxed),
            1);
    }
}

// 验证配置多个工作线程后，至少两个任务能够真正同时进入执行阶段。
TEST(ThreadPoolTest, MultipleWorkersCanExecuteTasksConcurrently)
{
    ThreadPool pool(2);
    std::atomic<int> startedTaskCount{0};
    std::promise<void> bothTasksStartedPromise;
    std::future<void> bothTasksStartedFuture = bothTasksStartedPromise.get_future();
    std::promise<void> releaseTasksPromise;
    std::shared_future<void> releaseTasksFuture =
        releaseTasksPromise.get_future().share();

    // 两个任务启动后都阻塞在同一闸门；若只有一个工作线程，第二个任务
    // 无法启动，bothTasksStartedFuture 就会超时。
    auto blockingTask = [
        &startedTaskCount,
        &bothTasksStartedPromise,
        releaseTasksFuture
    ]() {
        const int startedCount =
            startedTaskCount.fetch_add(1, std::memory_order_acq_rel) + 1;

        if (startedCount == 2) {
            bothTasksStartedPromise.set_value();
        }

        releaseTasksFuture.wait();
    };

    pool.post(blockingTask);
    pool.post(blockingTask);

    const std::future_status status =
        bothTasksStartedFuture.wait_for(kTaskTimeout);

    // 先释放任务，避免断言失败时线程池析构被阻塞。
    releaseTasksPromise.set_value();

    ASSERT_EQ(status, std::future_status::ready);
    EXPECT_EQ(startedTaskCount.load(std::memory_order_acquire), 2);
}

} // namespace
} // namespace engineeringlab::infrastructure
