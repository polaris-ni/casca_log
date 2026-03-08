/**
 * @author Polaris
 * @date 2026/3/8
 */

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "clog_rwlock.h"
#include "test_util.h"

class CLogRwLockTest : public testing::Test
{
protected:
    static const char *process;
    void SetUp() override
    {
        CLogTest::CLogMemLeakDetect::start(nullptr, nullptr);
    }

    void TearDown() override
    {
        CLogTest::CLogMemLeakDetect::end();
    }
};

TEST_F(CLogRwLockTest, InitDestroy)
{
    clog_rwlock_t *lock = clog_rwlock_create();
    ASSERT_NE(lock, nullptr);
    clog_rwlock_destroy(&lock);
}

TEST_F(CLogRwLockTest, NullPointerSafety)
{
    clog_rwlock_destroy(nullptr);
    EXPECT_NE(clog_rwlock_rd_lock(nullptr), CLOG_SUCCESS);
    EXPECT_NE(clog_rwlock_wr_lock(nullptr), CLOG_SUCCESS);
    clog_rwlock_rd_unlock(nullptr);
    clog_rwlock_wr_unlock(nullptr);
}

TEST_F(CLogRwLockTest, MultipleReadersConcurrent)
{
    clog_rwlock_t *lock = clog_rwlock_create();
    ASSERT_NE(lock, nullptr);

    std::atomic<int> reader_count{0};
    std::vector<std::thread> readers;
    constexpr int num_readers = 10;
    readers.reserve(num_readers);

    for (int i = 0; i < num_readers; ++i) {
        readers.emplace_back(
            [&]
            {
                ASSERT_EQ(clog_rwlock_rd_lock(lock), CLOG_SUCCESS);
                reader_count.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                clog_rwlock_rd_unlock(lock);
            });
    }

    for (auto &t : readers) {
        t.join();
    }
    EXPECT_EQ(reader_count.load(), num_readers);
    clog_rwlock_destroy(&lock);
}

TEST_F(CLogRwLockTest, WriterExclusion)
{
    clog_rwlock_t *lock = clog_rwlock_create();
    ASSERT_NE(lock, nullptr);

    std::atomic<bool> writer1_holding{false};
    std::atomic<bool> writer2_entered{false};

    std::thread writer1(
        [&]
        {
            ASSERT_EQ(clog_rwlock_wr_lock(lock), CLOG_SUCCESS);
            writer1_holding = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            writer1_holding = false;
            clog_rwlock_wr_unlock(lock);
        });

    std::thread writer2(
        [&]
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ASSERT_EQ(clog_rwlock_wr_lock(lock), CLOG_SUCCESS);
            writer2_entered = writer1_holding.load();
            clog_rwlock_wr_unlock(lock);
        });

    writer1.join();
    writer2.join();
    EXPECT_FALSE(writer2_entered);
    clog_rwlock_destroy(&lock);
}

TEST_F(CLogRwLockTest, ReaderWriterExclusion)
{
    clog_rwlock_t *lock = clog_rwlock_create();
    ASSERT_NE(lock, nullptr);

    std::atomic<bool> reader_holding{false};
    std::atomic<bool> writer_entered{false};

    std::thread reader(
        [&]
        {
            ASSERT_EQ(clog_rwlock_rd_lock(lock), CLOG_SUCCESS);
            reader_holding = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            reader_holding = false;
            clog_rwlock_rd_unlock(lock);
        });

    std::thread writer(
        [&]
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ASSERT_EQ(clog_rwlock_wr_lock(lock), CLOG_SUCCESS);
            writer_entered = reader_holding.load();
            clog_rwlock_wr_unlock(lock);
        });

    reader.join();
    writer.join();
    EXPECT_FALSE(writer_entered);
    clog_rwlock_destroy(&lock);
}
