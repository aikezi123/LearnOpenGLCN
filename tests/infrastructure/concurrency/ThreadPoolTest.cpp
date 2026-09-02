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

namespace learnopengl::infrastructure {
namespace {

using namespace std::chrono_literals;

constexpr auto kTaskTimeout = 5s;

static_assert(!std::is_copy_constructible_v<ThreadPool>);
static_assert(!std::is_copy_assignable_v<ThreadPool>);
static_assert(!std::is_move_constructible_v<ThreadPool>);
static_assert(!std::is_move_assignable_v<ThreadPool>);

TEST(ThreadPoolTest, ConstructionRejectsZeroWorkers)
{
    EXPECT_THROW(ThreadPool pool(0), std::invalid_argument);
}

TEST(ThreadPoolTest, PostExecutesTask)
{
    ThreadPool pool(1);
    std::promise<void> completedPromise;
    std::future<void> completedFuture = completedPromise.get_future();

    pool.post([&completedPromise]() {
        completedPromise.set_value();
    });

    EXPECT_EQ(completedFuture.wait_for(kTaskTimeout), std::future_status::ready);
}

TEST(ThreadPoolTest, PostedTaskExecutesExactlyOnce)
{
    ThreadPool pool(2);
    std::atomic<int> executionCount{0};

    pool.post([&executionCount]() {
        executionCount.fetch_add(1, std::memory_order_relaxed);
    });

    pool.shutdown();

    EXPECT_EQ(executionCount.load(std::memory_order_relaxed), 1);
}

TEST(ThreadPoolTest, ThrowingPostedTaskDoesNotStopWorker)
{
    ThreadPool pool(1);
    std::promise<void> completedPromise;
    std::future<void> completedFuture = completedPromise.get_future();

    pool.post([]() {
        throw std::runtime_error("ignored task failure");
    });
    pool.post([&completedPromise]() {
        completedPromise.set_value();
    });

    EXPECT_EQ(completedFuture.wait_for(kTaskTimeout), std::future_status::ready);
}

TEST(ThreadPoolTest, SubmitReturnsTaskResult)
{
    ThreadPool pool(1);

    std::future<int> resultFuture = pool.submit([]() {
        return 42;
    });

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_EQ(resultFuture.get(), 42);
}

TEST(ThreadPoolTest, SubmitPassesMultipleArguments)
{
    ThreadPool pool(1);

    std::future<std::string> resultFuture = pool.submit(
        [](std::string prefix, int value) {
            return prefix + std::to_string(value);
        },
        std::string{"result="},
        42);

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_EQ(resultFuture.get(), "result=42");
}

TEST(ThreadPoolTest, SubmitSupportsVoidResult)
{
    ThreadPool pool(1);
    std::atomic<int> result{0};

    std::future<void> completedFuture = pool.submit(
        [&result](int lhs, int rhs) {
            result.store(lhs + rhs, std::memory_order_relaxed);
        },
        20,
        22);

    ASSERT_EQ(completedFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_NO_THROW(completedFuture.get());
    EXPECT_EQ(result.load(std::memory_order_relaxed), 42);
}

TEST(ThreadPoolTest, SubmitSupportsMoveOnlyCallable)
{
    ThreadPool pool(1);

    std::future<int> resultFuture = pool.submit(
        [value = std::make_unique<int>(42)]() {
            return *value;
        });

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_EQ(resultFuture.get(), 42);
}

TEST(ThreadPoolTest, SubmitSupportsMoveOnlyArgument)
{
    ThreadPool pool(1);

    std::future<int> resultFuture = pool.submit(
        [](std::unique_ptr<int> value) {
            return *value;
        },
        std::make_unique<int>(42));

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_EQ(resultFuture.get(), 42);
}

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

TEST(ThreadPoolTest, SubmitPropagatesTaskExceptionThroughFuture)
{
    ThreadPool pool(1);

    std::future<int> resultFuture = pool.submit([]() -> int {
        throw std::runtime_error("task failed");
    });

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);

    try {
        static_cast<void>(resultFuture.get());
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& exception) {
        EXPECT_STREQ(exception.what(), "task failed");
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }
}

TEST(ThreadPoolTest, EmptyPostTaskIsRejected)
{
    ThreadPool pool(1);
    ThreadPool::Task emptyTask;

    EXPECT_THROW(pool.post(std::move(emptyTask)), std::invalid_argument);
}

TEST(ThreadPoolTest, EmptySubmitTaskPropagatesBadFunctionCallThroughFuture)
{
    ThreadPool pool(1);
    std::function<int()> emptyTask;

    std::future<int> resultFuture = pool.submit(std::move(emptyTask));

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_THROW(static_cast<void>(resultFuture.get()), std::bad_function_call);
}

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

    pool.shutdown();

    EXPECT_EQ(completedTaskCount.load(std::memory_order_relaxed), kTaskCount);
}

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

    EXPECT_EQ(completedTaskCount.load(std::memory_order_relaxed), kTaskCount);
}

TEST(ThreadPoolTest, ShutdownCanBeCalledMoreThanOnce)
{
    ThreadPool pool(1);

    EXPECT_NO_THROW(pool.shutdown());
    EXPECT_NO_THROW(pool.shutdown());
}

TEST(ThreadPoolTest, ConcurrentShutdownCallsAreSafe)
{
    ThreadPool pool(2);
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

    ASSERT_EQ(firstShutdown.wait_for(kTaskTimeout), std::future_status::ready);
    ASSERT_EQ(secondShutdown.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_NO_THROW(firstShutdown.get());
    EXPECT_NO_THROW(secondShutdown.get());
}

TEST(ThreadPoolTest, ShutdownRejectsNewTasks)
{
    ThreadPool pool(1);
    pool.shutdown();

    EXPECT_THROW(pool.post([]() {}), std::runtime_error);
    EXPECT_THROW(pool.submit([]() { return 1; }), std::runtime_error);
}

TEST(ThreadPoolTest, MultipleProducersExecuteEveryTaskExactlyOnce)
{
    constexpr int kProducerCount = 4;
    constexpr int kTasksPerProducer = 100;
    constexpr int kTotalTaskCount = kProducerCount * kTasksPerProducer;

    ThreadPool pool(4);
    std::array<std::atomic<int>, kTotalTaskCount> executionCounts;

    for (std::atomic<int>& count : executionCounts) {
        count.store(0, std::memory_order_relaxed);
    }

    std::vector<std::thread> producers;
    producers.reserve(kProducerCount);

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

    pool.shutdown();

    for (int taskIndex = 0; taskIndex < kTotalTaskCount; ++taskIndex) {
        SCOPED_TRACE("taskIndex=" + std::to_string(taskIndex));
        EXPECT_EQ(
            executionCounts[taskIndex].load(std::memory_order_relaxed),
            1);
    }
}

TEST(ThreadPoolTest, MultipleWorkersCanExecuteTasksConcurrently)
{
    ThreadPool pool(2);
    std::atomic<int> startedTaskCount{0};
    std::promise<void> bothTasksStartedPromise;
    std::future<void> bothTasksStartedFuture = bothTasksStartedPromise.get_future();
    std::promise<void> releaseTasksPromise;
    std::shared_future<void> releaseTasksFuture =
        releaseTasksPromise.get_future().share();

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
} // namespace learnopengl::infrastructure
