/**
 * @author Polaris
 * @date  2025/11/22
 */
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "clog_mutex.h"
#include "test_util.h"

class CLogMutexTest : public ::testing::Test
{
protected:
    clog_mutex_t mutex = {};
    void SetUp() override
    {
        CLogTest::CLogMemLeakDetect::start(nullptr, nullptr);
        ASSERT_EQ(clog_mutex_init(&mutex), CLOG_SUCCESS);
    }

    void TearDown() override
    {
        EXPECT_EQ(clog_mutex_destroy(&mutex), CLOG_SUCCESS);
        CLogTest::CLogMemLeakDetect::end();
    }
};

TEST_F(CLogMutexTest, BasicFunctionalityInSingleThread)
{
    EXPECT_EQ(clog_mutex_lock(&mutex), CLOG_SUCCESS);
    EXPECT_EQ(clog_mutex_unlock(&mutex), CLOG_SUCCESS);

    EXPECT_EQ(clog_mutex_trylock(&mutex), CLOG_SUCCESS);
    EXPECT_EQ(clog_mutex_unlock(&mutex), CLOG_SUCCESS);

    EXPECT_EQ(clog_mutex_trylock(&mutex), CLOG_SUCCESS);
    EXPECT_EQ(clog_mutex_unlock(&mutex), CLOG_SUCCESS);
}

TEST_F(CLogMutexTest, ErrorHandlingInSingleThread)
{
    EXPECT_EQ(clog_mutex_init(nullptr), CLOG_INVALID_PARAM);
    EXPECT_EQ(clog_mutex_lock(nullptr), CLOG_INVALID_PARAM);
    EXPECT_EQ(clog_mutex_trylock(nullptr), CLOG_INVALID_PARAM);
    EXPECT_EQ(clog_mutex_unlock(nullptr), CLOG_INVALID_PARAM);
    EXPECT_EQ(clog_mutex_destroy(nullptr), CLOG_INVALID_PARAM);
}

TEST_F(CLogMutexTest, RaceConditionInMultiThread)
{
    constexpr int num_threads = 10;
    constexpr int increments_per_thread = 1000;
    volatile int shared_counter = 0;

    auto worker = [&]
    {
        for (int i = 0; i < increments_per_thread; ++i) {
            EXPECT_EQ(clog_mutex_lock(&mutex), CLOG_SUCCESS);
            shared_counter++;
            EXPECT_EQ(clog_mutex_unlock(&mutex), CLOG_SUCCESS);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(shared_counter, num_threads * increments_per_thread);
}

TEST_F(CLogMutexTest, TryLockInMultiThread)
{
    constexpr int num_threads = 5;
    std::atomic success_count{0};
    std::atomic start_flag{false};

    auto worker = [&]
    {
        while (!start_flag.load()) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        if (clog_mutex_trylock(&mutex) == CLOG_SUCCESS) {
            success_count.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            clog_mutex_unlock(&mutex);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }

    start_flag.store(true);

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_GE(success_count.load(), 1);
    EXPECT_LE(success_count.load(), num_threads);
}

TEST_F(CLogMutexTest, LongHoldInMultiThread)
{
    constexpr int num_threads = 5;
    std::atomic completed_threads{0};

    ASSERT_EQ(clog_mutex_lock(&mutex), CLOG_SUCCESS);

    auto worker = [&]
    {
        EXPECT_EQ(clog_mutex_lock(&mutex), CLOG_SUCCESS);
        completed_threads.fetch_add(1);
        EXPECT_EQ(clog_mutex_unlock(&mutex), CLOG_SUCCESS);
    };

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(clog_mutex_unlock(&mutex), CLOG_SUCCESS);

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(completed_threads.load(), num_threads);
}
