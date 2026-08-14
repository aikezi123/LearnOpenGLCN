#include <thread_pool/ThreadPool.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <stdexcept>
#include <utility>

namespace learnopengl::infrastructure {
namespace {

using namespace std::chrono_literals;

constexpr auto kTaskTimeout = 5s;

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

TEST(ThreadPoolTest, SubmitReturnsTaskResult)
{
    ThreadPool pool(1);

    std::future<int> resultFuture = pool.submit([]() {
        return 42;
    });

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_EQ(resultFuture.get(), 42);
}

TEST(ThreadPoolTest, SubmitPropagatesTaskExceptionThroughFuture)
{
    ThreadPool pool(1);

    std::future<int> resultFuture = pool.submit([]() -> int {
        throw std::runtime_error("task failed");
    });

    ASSERT_EQ(resultFuture.wait_for(kTaskTimeout), std::future_status::ready);
    EXPECT_THROW(static_cast<void>(resultFuture.get()), std::runtime_error);
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

TEST(ThreadPoolTest, ShutdownDrainsAcceptedTasks)
{
    ThreadPool pool(2);
    std::atomic<int> completedTaskCount{0};
    constexpr int kTaskCount = 32;

    for (int index = 0; index < kTaskCount; ++index) {
        pool.post([&completedTaskCount]() {
            completedTaskCount.fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.shutdown();

    EXPECT_EQ(completedTaskCount.load(std::memory_order_relaxed), kTaskCount);
}

TEST(ThreadPoolTest, ShutdownCanBeCalledMoreThanOnce)
{
    ThreadPool pool(1);

    pool.shutdown();
    pool.shutdown();
}

TEST(ThreadPoolTest, ShutdownRejectsNewTasks)
{
    ThreadPool pool(1);
    pool.shutdown();

    EXPECT_THROW(pool.post([]() {}), std::runtime_error);
    EXPECT_THROW(pool.submit([]() { return 1; }), std::runtime_error);
}

TEST(ThreadPoolTest, EmptyTasksAreRejected)
{
    ThreadPool pool(1);
    ThreadPool::Task emptyPostTask;
    std::function<int()> emptySubmitTask;

    EXPECT_THROW(pool.post(std::move(emptyPostTask)), std::runtime_error);
    EXPECT_THROW(pool.submit(std::move(emptySubmitTask)), std::runtime_error);
}

} // namespace
} // namespace learnopengl::infrastructure
