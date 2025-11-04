#include <gtest/gtest.h>
#include <queue>
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
