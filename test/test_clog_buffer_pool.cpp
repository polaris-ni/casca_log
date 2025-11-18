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
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(ClogBufferPoolTest, InitializeSuccess)
{
    clog_buffer_pool_t* pool = nullptr;
    const clog_res_e result = clog_buffer_pool_initialize(&pool, sizeof(uint32_t), 10, true, 50);
    EXPECT_EQ(result, CLOG_SUCCESS);
    EXPECT_EQ(clog_buffer_pool_get_current_capacity(pool), 10);
    EXPECT_TRUE(clog_buffer_pool_is_auto_manager(pool));
    EXPECT_EQ(clog_buffer_pool_get_state(pool), CLOG_BUFFER_POOL_STATE_RUNNING);
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_SUCCESS);
}

TEST_F(ClogBufferPoolTest, InitializeWithInvalidThreshold)
{
    clog_buffer_pool_t* pool = nullptr;
    clog_res_e result = clog_buffer_pool_initialize(&pool, sizeof(uint32_t), 10, true, 0);
    EXPECT_EQ(result, CLOG_INVALID_PARAM);
    result = clog_buffer_pool_initialize(&pool, sizeof(uint32_t), 10, true, 100);
    EXPECT_EQ(result, CLOG_INVALID_PARAM);
}

TEST_F(ClogBufferPoolTest, AcquireAndReleaseWithoutAutoManager)
{
    clog_buffer_pool_t* pool = nullptr;
    clog_buffer_pool_initialize(&pool, sizeof(uint32_t), 5, false, 0);

    void* entry1 = clog_buffer_pool_acquire(pool);
    void* entry2 = clog_buffer_pool_acquire(pool);
    void* entry3 = clog_buffer_pool_acquire(pool);
    void* entry4 = clog_buffer_pool_acquire(pool);
    void* entry5 = clog_buffer_pool_acquire(pool);
    void* entry6 = clog_buffer_pool_acquire(pool);
    ASSERT_NE(entry1, nullptr);
    ASSERT_NE(entry2, nullptr);
    ASSERT_NE(entry3, nullptr);
    ASSERT_NE(entry4, nullptr);
    ASSERT_NE(entry5, nullptr);
    ASSERT_NE(entry6, nullptr);

    EXPECT_EQ(clog_buffer_pool_get_current_capacity(pool), 5);

    EXPECT_EQ(clog_buffer_pool_release(pool, entry1), pool);
    EXPECT_EQ(clog_buffer_pool_release(pool, entry2), pool);
    EXPECT_EQ(clog_buffer_pool_release(pool, entry3), pool);
    EXPECT_EQ(clog_buffer_pool_release(pool, entry4), pool);
    EXPECT_EQ(clog_buffer_pool_release(pool, entry5), pool);
    EXPECT_EQ(clog_buffer_pool_release(pool, entry6), pool);
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_SUCCESS);
}

TEST_F(ClogBufferPoolTest, AutoExpandWhenFull)
{
    clog_buffer_pool_t* pool = nullptr;
    clog_buffer_pool_initialize(&pool, sizeof(uint32_t), 3, true, 50);

    void* entries[5];
    for (auto& entry : entries) {
        entry = clog_buffer_pool_acquire(pool);
        ASSERT_NE(entry, nullptr);
    }

    EXPECT_GE(clog_buffer_pool_get_current_capacity(pool), 3);

    for (auto& entry : entries) {
        EXPECT_EQ(clog_buffer_pool_release(pool, entry), pool);
    }
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_SUCCESS);
}

TEST_F(ClogBufferPoolTest, AutoShrinkWhenBelowThreshold)
{
    clog_buffer_pool_t* pool = nullptr;
    constexpr size_t init_capacity = 10;
    clog_buffer_pool_initialize(&pool, sizeof(uint32_t), init_capacity, true, 60);

    std::queue<void*> entries;
    for (size_t i = 0; i < init_capacity * 2; i++) {
        void* entry = clog_buffer_pool_acquire(pool);
        ASSERT_NE(entry, nullptr);
        entries.push(entry);
    }

    size_t capacity_before_release = clog_buffer_pool_get_current_capacity(pool);
    ASSERT_EQ(capacity_before_release, init_capacity * 2);

    constexpr size_t release_num = static_cast<size_t>(init_capacity * 2 * 0.4) - 1;

    for (size_t i = 0; i < release_num; i++) {
        EXPECT_EQ(clog_buffer_pool_release(pool, entries.front()), pool);
        entries.pop();
    }

    capacity_before_release = clog_buffer_pool_get_current_capacity(pool);
    ASSERT_EQ(capacity_before_release, init_capacity * 2);

    for (size_t i = 0; i < 2; i++) {
        EXPECT_EQ(clog_buffer_pool_release(pool, entries.front()), pool);
        entries.pop();
    }

    capacity_before_release = clog_buffer_pool_get_current_capacity(pool);
    ASSERT_LT(capacity_before_release, init_capacity * 2);

    while (!entries.empty()) {
        EXPECT_EQ(clog_buffer_pool_release(pool, entries.front()), pool);
        entries.pop();
    }
    capacity_before_release = clog_buffer_pool_get_current_capacity(pool);
    ASSERT_EQ(capacity_before_release, init_capacity);
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_SUCCESS);
}

TEST_F(ClogBufferPoolTest, ReleaseWhenFinalized)
{
    clog_buffer_pool_t* pool = nullptr;
    EXPECT_EQ(clog_buffer_pool_initialize(&pool, sizeof(uint32_t), 5, false, 0), CLOG_SUCCESS);
    void* entry = clog_buffer_pool_acquire(pool);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_NOT_COMPLETED);
    EXPECT_EQ(clog_buffer_pool_release(pool, entry), nullptr);
}

TEST_F(ClogBufferPoolTest, GetState)
{
    clog_buffer_pool_t* pool = nullptr;
    EXPECT_EQ(clog_buffer_pool_get_state(pool), CLOG_BUFFER_POOL_STATE_DISABLED);
    EXPECT_EQ(clog_buffer_pool_initialize(&pool, sizeof(uint32_t), 5, false, 0), CLOG_SUCCESS);
    EXPECT_EQ(clog_buffer_pool_get_state(pool), CLOG_BUFFER_POOL_STATE_RUNNING);
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_SUCCESS);
}

TEST_F(ClogBufferPoolTest, GetCurrentCapacity)
{
    clog_buffer_pool_t* pool = nullptr;
    EXPECT_EQ(clog_buffer_pool_get_current_capacity(pool), 0);
    EXPECT_EQ(clog_buffer_pool_initialize(&pool, sizeof(uint32_t), 7, false, 0), CLOG_SUCCESS);
    EXPECT_EQ(clog_buffer_pool_get_current_capacity(pool), 7);
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_SUCCESS);
}

TEST_F(ClogBufferPoolTest, IsAutoManager)
{
    clog_buffer_pool_t* pool = nullptr;
    EXPECT_EQ(clog_buffer_pool_initialize(&pool, sizeof(uint32_t), 5, false, 0), CLOG_SUCCESS);
    EXPECT_FALSE(clog_buffer_pool_is_auto_manager(pool));
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_SUCCESS);
    EXPECT_EQ(clog_buffer_pool_initialize(&pool, sizeof(uint32_t), 5, true, 50), CLOG_SUCCESS);
    EXPECT_TRUE(clog_buffer_pool_is_auto_manager(pool));
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_SUCCESS);
}

TEST_F(ClogBufferPoolTest, MultiThreadAcquireAndRelease)
{
    clog_buffer_pool_t* pool = nullptr;
    constexpr size_t init_capacity = 20;
    constexpr uint8_t threshold = 50;
    constexpr int thread_count = 8;
    constexpr int operations_per_thread = 100;

    EXPECT_EQ(clog_buffer_pool_initialize(&pool, sizeof(uint32_t), init_capacity, true, threshold), CLOG_SUCCESS);

    std::vector<std::vector<void*>> thread_entries(thread_count);

    auto worker = [&](int thread_id)
    {
        for (int i = 0; i < operations_per_thread; ++i) {
            void* entry = clog_buffer_pool_acquire(pool);
            ASSERT_NE(entry, nullptr);

            thread_entries[thread_id].push_back(entry);

            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        for (void* entry : thread_entries[thread_id]) {
            EXPECT_EQ(clog_buffer_pool_release(pool, entry), pool);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(clog_buffer_pool_get_state(pool), CLOG_BUFFER_POOL_STATE_RUNNING);
    EXPECT_TRUE(clog_buffer_pool_is_auto_manager(pool));
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_SUCCESS);
}

TEST_F(ClogBufferPoolTest, MultiThreadAutoExpandAndShrink)
{
    clog_buffer_pool_t* pool = nullptr;
    constexpr size_t init_capacity = 5;
    constexpr uint8_t threshold = 60;
    constexpr int thread_count = 4;
    constexpr int operations_per_thread = 50;

    EXPECT_EQ(clog_buffer_pool_initialize(&pool, sizeof(uint32_t), init_capacity, true, threshold), CLOG_SUCCESS);

    std::atomic<int> total_acquired{0};
    std::vector<std::vector<void*>> thread_entries(thread_count);

    auto worker = [&](const int index)
    {
        for (int i = 0; i < operations_per_thread; ++i) {
            void* entry = clog_buffer_pool_acquire(pool);
            ASSERT_NE(entry, nullptr);
            thread_entries[index].push_back(entry);
            ++total_acquired;
            std::this_thread::sleep_for(std::chrono::microseconds(5));
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    size_t current_capacity = clog_buffer_pool_get_current_capacity(pool);
    EXPECT_GE(current_capacity, init_capacity);

    for (auto& entries : thread_entries) {
        for (void* entry : entries) {
            EXPECT_EQ(clog_buffer_pool_release(pool, entry), pool);
        }
    }

    current_capacity = clog_buffer_pool_get_current_capacity(pool);
    EXPECT_LE(current_capacity, 80); /* 4 * 50 * (1 - 0.6) */
    EXPECT_GT(current_capacity, init_capacity);
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_SUCCESS);
}

TEST_F(ClogBufferPoolTest, MultiThreadUniqueEntryCheck)
{
    clog_buffer_pool_t* pool = nullptr;
    constexpr size_t init_capacity = 10;
    constexpr int thread_count = 8;
    constexpr int operations_per_thread = 50;

    EXPECT_EQ(clog_buffer_pool_initialize(&pool, sizeof(uint32_t), init_capacity, true, 50), CLOG_SUCCESS);
    std::unordered_map<void*, std::thread::id> entry_thread_map;
    std::mutex map_mutex;
    std::atomic<bool> duplicate_detected{false};
    std::atomic<int> total_acquired{0};

    auto worker = [&](int)
    {
        std::vector<void*> local_entries;
        local_entries.reserve(operations_per_thread);

        for (int i = 0; i < operations_per_thread; ++i) {
            if (duplicate_detected.load()) {
                break;
            }

            void* entry = clog_buffer_pool_acquire(pool);
            ASSERT_NE(entry, nullptr);
            {
                std::lock_guard lock(map_mutex);
                const auto result = entry_thread_map.insert({entry, std::this_thread::get_id()});
                if (!result.second) {
                    duplicate_detected.store(true);
                    FAIL() << "Duplicate entry detected: " << entry << " already held by thread "
                           << entry_thread_map[entry] << " and requested by thread " << std::this_thread::get_id();
                }
            }

            local_entries.push_back(entry);
            total_acquired.fetch_add(1);

            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        for (void* entry : local_entries) {
            {
                std::lock_guard lock(map_mutex);
                auto it = entry_thread_map.find(entry);
                if (it != entry_thread_map.end()) {
                    entry_thread_map.erase(it);
                }
            }

            EXPECT_EQ(clog_buffer_pool_release(pool, entry), pool);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_FALSE(duplicate_detected.load());
    EXPECT_EQ(entry_thread_map.size(), 0);
    EXPECT_EQ(clog_buffer_pool_finalize(pool), CLOG_SUCCESS);
}
