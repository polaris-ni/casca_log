/**
 * @auther polaris
 * @date  2025/11/04
 */
#include <chrono>
#include <gtest/gtest.h>
#include <queue>
#include <thread>
#include "clog_buffer_pool.h"

class ClogBufferPoolTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        clog_buffer_pool_finalize();
    }

    void TearDown() override
    {
        clog_buffer_pool_finalize();
    }
};

TEST_F(ClogBufferPoolTest, InitializeSuccess)
{
    const clog_res_e result = clog_buffer_pool_initialize(10, true, 50);
    EXPECT_EQ(result, CLOG_SUCCESS);
    EXPECT_EQ(clog_buffer_pool_get_current_capacity(), 10);
    EXPECT_TRUE(clog_buffer_pool_is_auto_manager());
    EXPECT_EQ(clog_buffer_pool_get_state(), CLOG_BUFFER_POOL_STATE_RUNNING);
}

TEST_F(ClogBufferPoolTest, InitializeWithInvalidThreshold)
{
    clog_res_e result = clog_buffer_pool_initialize(10, true, 0);
    EXPECT_EQ(result, CLOG_INVALID_PARAM);
    result = clog_buffer_pool_initialize(10, true, 100);
    EXPECT_EQ(result, CLOG_INVALID_PARAM);
}

TEST_F(ClogBufferPoolTest, AcquireAndReleaseWithoutAutoManager)
{
    clog_buffer_pool_initialize(5, false, 0);

    clog_entry_t* entry1 = clog_buffer_pool_acquire();
    clog_entry_t* entry2 = clog_buffer_pool_acquire();
    clog_entry_t* entry3 = clog_buffer_pool_acquire();
    clog_entry_t* entry4 = clog_buffer_pool_acquire();
    clog_entry_t* entry5 = clog_buffer_pool_acquire();
    clog_entry_t* entry6 = clog_buffer_pool_acquire();
    ASSERT_NE(entry1, nullptr);
    ASSERT_NE(entry2, nullptr);
    ASSERT_NE(entry3, nullptr);
    ASSERT_NE(entry4, nullptr);
    ASSERT_NE(entry5, nullptr);
    ASSERT_NE(entry6, nullptr);

    EXPECT_EQ(clog_buffer_pool_get_current_capacity(), 5);

    clog_buffer_pool_release(entry1);
    clog_buffer_pool_release(entry2);
    clog_buffer_pool_release(entry3);
    clog_buffer_pool_release(entry4);
    clog_buffer_pool_release(entry5);
    clog_buffer_pool_release(entry6);
}

// 测试缓冲池扩展功能
TEST_F(ClogBufferPoolTest, AutoExpandWhenFull)
{
    clog_buffer_pool_initialize(3, true, 50);

    // 获取所有初始缓冲区
    clog_entry_t* entries[5];
    for (auto& entry : entries) {
        entry = clog_buffer_pool_acquire();
        ASSERT_NE(entry, nullptr);
    }

    EXPECT_GE(clog_buffer_pool_get_current_capacity(), 3);

    for (auto& entry : entries) {
        clog_buffer_pool_release(entry);
    }
}

TEST_F(ClogBufferPoolTest, AutoShrinkWhenBelowThreshold)
{
    constexpr size_t init_capacity = 10;
    clog_buffer_pool_initialize(init_capacity, true, 60);

    std::queue<clog_entry_t*> entries;
    for (size_t i = 0; i < init_capacity * 2; i++) {
        clog_entry_t* entry = clog_buffer_pool_acquire();
        ASSERT_NE(entry, nullptr);
        entries.push(entry);
    }

    size_t capacity_before_release = clog_buffer_pool_get_current_capacity();
    ASSERT_EQ(capacity_before_release, init_capacity * 2);

    constexpr size_t release_num = init_capacity * 2 * 0.4 - 1;

    for (size_t i = 0; i < release_num; i++) {
        clog_buffer_pool_release(entries.front());
        entries.pop();
    }

    capacity_before_release = clog_buffer_pool_get_current_capacity();
    ASSERT_EQ(capacity_before_release, init_capacity * 2);

    for (size_t i = 0; i < 2; i++) {
        clog_buffer_pool_release(entries.front());
        entries.pop();
    }

    capacity_before_release = clog_buffer_pool_get_current_capacity();
    ASSERT_LT(capacity_before_release, init_capacity * 2);

    while (!entries.empty()) {
        clog_buffer_pool_release(entries.front());
        entries.pop();
    }
    capacity_before_release = clog_buffer_pool_get_current_capacity();
    ASSERT_EQ(capacity_before_release, init_capacity);
}

TEST_F(ClogBufferPoolTest, AcquireWhenDisabled)
{
    clog_buffer_pool_initialize(5, false, 0);
    clog_buffer_pool_finalize(); // 禁用缓冲池

    clog_entry_t* entry = clog_buffer_pool_acquire();
    ASSERT_NE(entry, nullptr);

    clog_buffer_pool_release(entry);
}

TEST_F(ClogBufferPoolTest, GetState)
{
    EXPECT_EQ(clog_buffer_pool_get_state(), CLOG_BUFFER_POOL_STATE_DISABLED);
    clog_buffer_pool_initialize(5, false, 0);
    EXPECT_EQ(clog_buffer_pool_get_state(), CLOG_BUFFER_POOL_STATE_RUNNING);
    clog_buffer_pool_finalize();
    EXPECT_EQ(clog_buffer_pool_get_state(), CLOG_BUFFER_POOL_STATE_DISABLED);
}

TEST_F(ClogBufferPoolTest, GetCurrentCapacity)
{
    EXPECT_EQ(clog_buffer_pool_get_current_capacity(), 0);
    clog_buffer_pool_initialize(7, false, 0);
    EXPECT_EQ(clog_buffer_pool_get_current_capacity(), 7);
    clog_buffer_pool_finalize();
    EXPECT_EQ(clog_buffer_pool_get_current_capacity(), 0);
}

TEST_F(ClogBufferPoolTest, IsAutoManager)
{
    clog_buffer_pool_initialize(5, false, 0);
    EXPECT_FALSE(clog_buffer_pool_is_auto_manager());
    clog_buffer_pool_finalize();
    clog_buffer_pool_initialize(5, true, 50);
    EXPECT_TRUE(clog_buffer_pool_is_auto_manager());
}

// 多线程测试用例
TEST_F(ClogBufferPoolTest, MultiThreadAcquireAndRelease)
{
    constexpr size_t init_capacity = 20;
    constexpr uint8_t threshold = 50;
    constexpr int thread_count = 8;
    constexpr int operations_per_thread = 100;

    // 初始化缓冲池
    clog_buffer_pool_initialize(init_capacity, true, threshold);

    // 用于存储每个线程处理的entry
    std::vector<std::vector<clog_entry_t*>> thread_entries(thread_count);

    // 线程函数：执行获取和释放操作
    auto worker = [&](int thread_id)
    {
        for (int i = 0; i < operations_per_thread; ++i) {
            // 获取entry
            clog_entry_t* entry = clog_buffer_pool_acquire();
            ASSERT_NE(entry, nullptr);

            // 将entry存储起来
            thread_entries[thread_id].push_back(entry);

            // 模拟一些处理时间
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        // 释放所有获取的entry
        for (clog_entry_t* entry : thread_entries[thread_id]) {
            clog_buffer_pool_release(entry);
        }
    };

    // 创建并启动线程
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(worker, i);
    }

    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }

    // 验证最终状态
    EXPECT_EQ(clog_buffer_pool_get_state(), CLOG_BUFFER_POOL_STATE_RUNNING);
    EXPECT_TRUE(clog_buffer_pool_is_auto_manager());
}

// 测试多线程环境下的自动扩展和收缩
TEST_F(ClogBufferPoolTest, MultiThreadAutoExpandAndShrink)
{
    constexpr size_t init_capacity = 5;
    constexpr uint8_t threshold = 60;
    constexpr int thread_count = 4;
    constexpr int operations_per_thread = 50;

    clog_buffer_pool_initialize(init_capacity, true, threshold);

    std::atomic<int> total_acquired{0};
    std::vector<std::vector<clog_entry_t*>> thread_entries(thread_count);

    auto worker = [&](const int index)
    {
        for (int i = 0; i < operations_per_thread; ++i) {
            clog_entry_t* entry = clog_buffer_pool_acquire();
            ASSERT_NE(entry, nullptr);
            thread_entries[index].push_back(entry);
            ++total_acquired;
            std::this_thread::sleep_for(std::chrono::microseconds(5));
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    size_t current_capacity = clog_buffer_pool_get_current_capacity();
    EXPECT_GE(current_capacity, init_capacity);

    for (auto& entries : thread_entries) {
        for (clog_entry_t* entry : entries) {
            clog_buffer_pool_release(entry);
        }
    }

    current_capacity = clog_buffer_pool_get_current_capacity();
    EXPECT_LE(current_capacity, 80); /* 4 * 50 * (1 - 0.6) */
    EXPECT_GT(current_capacity, init_capacity);
}

TEST_F(ClogBufferPoolTest, MultiThreadUniqueEntryCheck)
{
    constexpr size_t init_capacity = 10;
    constexpr int thread_count = 8;
    constexpr int operations_per_thread = 50;

    clog_buffer_pool_initialize(init_capacity, true, 50);
    std::unordered_map<clog_entry_t*, std::thread::id> entry_thread_map;
    std::mutex map_mutex;
    std::atomic<bool> duplicate_detected{false};
    std::atomic<int> total_acquired{0};

    auto worker = [&](int) {
        std::vector<clog_entry_t*> local_entries;
        local_entries.reserve(operations_per_thread);

        for (int i = 0; i < operations_per_thread; ++i) {
            if (duplicate_detected.load()) {
                break;
            }

            clog_entry_t* entry = clog_buffer_pool_acquire();
            ASSERT_NE(entry, nullptr);
            {
                std::lock_guard lock(map_mutex);
                const auto result = entry_thread_map.insert({entry, std::this_thread::get_id()});
                if (!result.second) {
                    duplicate_detected.store(true);
                    FAIL() << "Duplicate entry detected: " << entry
                           << " already held by thread " << entry_thread_map[entry]
                           << " and requested by thread " << std::this_thread::get_id();
                }
            }

            local_entries.push_back(entry);
            total_acquired.fetch_add(1);

            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        for (clog_entry_t* entry : local_entries) {
            {
                std::lock_guard lock(map_mutex);
                auto it = entry_thread_map.find(entry);
                if (it != entry_thread_map.end()) {
                    entry_thread_map.erase(it);
                }
            }

            clog_buffer_pool_release(entry);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_FALSE(duplicate_detected.load());
    EXPECT_EQ(entry_thread_map.size(), 0);
}

