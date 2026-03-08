/**
 * @author Polaris
 * @date  2026/03/08
 */
#include <gtest/gtest.h>
#include "clog_queue.h"
#include "test_util.h"

class CLogQueueTest : public testing::Test
{
protected:
    clog_queue_t *queue = nullptr;

    void SetUp() override
    {
        queue = nullptr;
        CLogTest::CLogMemLeakDetect::start(nullptr, nullptr);
    }

    void TearDown() override
    {
        if (queue != nullptr) {
            clog_queue_destroy(&queue);
            queue = nullptr;
        }
        CLogTest::CLogMemLeakDetect::end();
    }

    static void CLogQueueTestNoFree(void *ptr)
    {
        CLOG_UNUSED_VAR(ptr);
    }
};

TEST_F(CLogQueueTest, CreateAndDestroy)
{
    EXPECT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);
    ASSERT_NE(queue, nullptr);
    clog_queue_destroy(&queue);
}

TEST_F(CLogQueueTest, CreateWithNullPointer)
{
    EXPECT_EQ(clog_queue_create(nullptr, CLogQueueTestNoFree), CLOG_INVALID_PARAM);
}

TEST_F(CLogQueueTest, EnqueueNormal)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    constexpr std::uintptr_t data = 12345;
    EXPECT_EQ(clog_queue_enqueue(queue, data), CLOG_SUCCESS);

    EXPECT_EQ(clog_queue_size(queue), 1u);
    EXPECT_FALSE(clog_queue_is_empty(queue));
}

TEST_F(CLogQueueTest, EnqueueWithNullQueue)
{
    constexpr std::uintptr_t data = 12345;
    EXPECT_EQ(clog_queue_enqueue(nullptr, data), CLOG_INVALID_PARAM);
}

TEST_F(CLogQueueTest, MultipleEnqueue)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    for (std::size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(clog_queue_enqueue(queue, i), CLOG_SUCCESS);
    }

    EXPECT_EQ(clog_queue_size(queue), 10u);
    EXPECT_FALSE(clog_queue_is_empty(queue));
}

TEST_F(CLogQueueTest, DequeueFromEmptyQueue)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    std::uintptr_t data;
    EXPECT_EQ(clog_queue_dequeue(queue, &data), CLOG_TARGET_NOT_FOUND);
}

TEST_F(CLogQueueTest, EnqueueAndDequeue)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    constexpr std::uintptr_t data_in = 54321;
    EXPECT_EQ(clog_queue_enqueue(queue, data_in), CLOG_SUCCESS);

    std::uintptr_t data_out;
    EXPECT_EQ(clog_queue_dequeue(queue, &data_out), CLOG_SUCCESS);
    EXPECT_EQ(data_in, data_out);

    EXPECT_TRUE(clog_queue_is_empty(queue));
    EXPECT_EQ(clog_queue_size(queue), 0u);
}

TEST_F(CLogQueueTest, PeekNormal)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    constexpr std::uintptr_t data = 99999;
    EXPECT_EQ(clog_queue_enqueue(queue, data), CLOG_SUCCESS);

    std::uintptr_t peeked_data;
    EXPECT_EQ(clog_queue_peek(queue, &peeked_data), CLOG_SUCCESS);
    EXPECT_EQ(data, peeked_data);

    std::uintptr_t dequeued_data;
    EXPECT_EQ(clog_queue_dequeue(queue, &dequeued_data), CLOG_SUCCESS);
    EXPECT_EQ(dequeued_data, data);

    EXPECT_EQ(clog_queue_size(queue), 0u);
}

TEST_F(CLogQueueTest, PeekWithNullQueue)
{
    std::uintptr_t data;
    EXPECT_EQ(clog_queue_peek(nullptr, &data), CLOG_INVALID_PARAM);
}

TEST_F(CLogQueueTest, PeekWithNullData)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);
    EXPECT_EQ(clog_queue_enqueue(queue, 123), CLOG_SUCCESS);
    EXPECT_EQ(clog_queue_peek(queue, nullptr), CLOG_INVALID_PARAM);
}

TEST_F(CLogQueueTest, PeekEmptyQueue)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    std::uintptr_t data;
    EXPECT_EQ(clog_queue_peek(queue, &data), CLOG_TARGET_NOT_FOUND);
}

TEST_F(CLogQueueTest, IsEmpty)
{
    EXPECT_TRUE(clog_queue_is_empty(nullptr));

    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);
    EXPECT_TRUE(clog_queue_is_empty(queue));
    EXPECT_EQ(clog_queue_size(queue), 0u);

    constexpr std::uintptr_t data = 1;
    EXPECT_EQ(clog_queue_enqueue(queue, data), CLOG_SUCCESS);
    EXPECT_FALSE(clog_queue_is_empty(queue));
    EXPECT_EQ(clog_queue_size(queue), 1u);

    std::uintptr_t data_out;
    EXPECT_EQ(clog_queue_dequeue(queue, &data_out), CLOG_SUCCESS);
    EXPECT_TRUE(clog_queue_is_empty(queue));
    EXPECT_EQ(clog_queue_size(queue), 0u);
}

TEST_F(CLogQueueTest, Size)
{
    EXPECT_EQ(clog_queue_size(nullptr), 0u);

    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);
    EXPECT_EQ(clog_queue_size(queue), 0u);

    for (std::size_t i = 0; i < 5; ++i) {
        clog_queue_enqueue(queue, i);
    }
    EXPECT_EQ(clog_queue_size(queue), 5u);

    std::uintptr_t data;
    clog_queue_dequeue(queue, &data);
    EXPECT_EQ(clog_queue_size(queue), 4u);
}

TEST_F(CLogQueueTest, Clear)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    for (std::size_t i = 0; i < 100; ++i) {
        clog_queue_enqueue(queue, i);
    }
    EXPECT_EQ(clog_queue_size(queue), 100u);

    clog_queue_clear(queue);
    EXPECT_TRUE(clog_queue_is_empty(queue));
    EXPECT_EQ(clog_queue_size(queue), 0u);
}

TEST_F(CLogQueueTest, ClearWithNullQueue)
{
    clog_queue_clear(nullptr);
}

TEST_F(CLogQueueTest, DestroyWithNullPointer)
{
    clog_queue_destroy(nullptr);
}

TEST_F(CLogQueueTest, DestroyNullQueue)
{
    clog_queue_t *null_queue = nullptr;
    clog_queue_destroy(&null_queue);
}

TEST_F(CLogQueueTest, LargeDataOperations)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    constexpr std::size_t count = 10000;
    for (std::size_t i = 0; i < count; ++i) {
        EXPECT_EQ(clog_queue_enqueue(queue, i), CLOG_SUCCESS) << "Failed at iteration " << i;
    }

    EXPECT_EQ(count, clog_queue_size(queue));

    for (std::size_t i = 0; i < count; ++i) {
        std::uintptr_t data_out;
        EXPECT_EQ(clog_queue_dequeue(queue, &data_out), CLOG_SUCCESS);
        EXPECT_EQ(i, data_out);
    }

    EXPECT_TRUE(clog_queue_is_empty(queue));
    EXPECT_EQ(0u, clog_queue_size(queue));
}

TEST_F(CLogQueueTest, FIFOOrder)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    constexpr std::size_t count = 100;
    for (std::size_t i = 0; i < count; ++i) {
        clog_queue_enqueue(queue, i);
    }

    for (std::size_t i = 0; i < count; ++i) {
        std::uintptr_t data;
        EXPECT_EQ(clog_queue_dequeue(queue, &data), CLOG_SUCCESS);
        EXPECT_EQ(i, data);
    }
}

TEST_F(CLogQueueTest, DeallocatorCalledOnClear)
{
    ASSERT_EQ(clog_queue_create(&queue, clog_sys_free), CLOG_SUCCESS);

    for (std::size_t i = 0; i < 10; ++i) {
        auto *data = static_cast<uintptr_t *>(clog_malloc(sizeof(uintptr_t)));
        ASSERT_NE(data, nullptr);
        *data = i;
        clog_queue_enqueue(queue, reinterpret_cast<uintptr_t>(data));
    }

    clog_queue_clear(queue);
    EXPECT_EQ(clog_queue_size(queue), 0u);
}

TEST_F(CLogQueueTest, MixedOperations)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    for (std::size_t i = 0; i < 50; ++i) {
        clog_queue_enqueue(queue, i);
    }
    EXPECT_EQ(clog_queue_size(queue), 50u);

    for (std::size_t i = 0; i < 25; ++i) {
        std::uintptr_t data;
        EXPECT_EQ(clog_queue_dequeue(queue, &data), CLOG_SUCCESS);
    }
    EXPECT_EQ(clog_queue_size(queue), 25u);

    for (std::size_t i = 50; i < 75; ++i) {
        clog_queue_enqueue(queue, i);
    }
    EXPECT_EQ(clog_queue_size(queue), 50u);

    for (std::size_t i = 0; i < 10; ++i) {
        std::uintptr_t data;
        EXPECT_EQ(clog_queue_dequeue(queue, &data), CLOG_SUCCESS);
    }
    EXPECT_EQ(clog_queue_size(queue), 40u);

    clog_queue_clear(queue);
    EXPECT_TRUE(clog_queue_is_empty(queue));
}

TEST_F(CLogQueueTest, SingleThreadBasicFunctionality)
{
    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    EXPECT_TRUE(clog_queue_is_empty(queue));
    EXPECT_EQ(clog_queue_size(queue), 0u);

    clog_queue_enqueue(queue, 1);
    clog_queue_enqueue(queue, 2);
    clog_queue_enqueue(queue, 3);

    EXPECT_FALSE(clog_queue_is_empty(queue));
    EXPECT_EQ(clog_queue_size(queue), 3u);

    std::uintptr_t data;
    EXPECT_EQ(clog_queue_dequeue(queue, &data), CLOG_SUCCESS);
    EXPECT_EQ(data, 1u);

    EXPECT_EQ(clog_queue_peek(queue, &data), CLOG_SUCCESS);
    EXPECT_EQ(data, 2u);
    EXPECT_EQ(clog_queue_size(queue), 2u);

    clog_queue_clear(queue);
    EXPECT_TRUE(clog_queue_is_empty(queue));
}

TEST_F(CLogQueueTest, ErrorHandling)
{
    EXPECT_EQ(clog_queue_create(nullptr, CLogQueueTestNoFree), CLOG_INVALID_PARAM);

    ASSERT_EQ(clog_queue_create(&queue, CLogQueueTestNoFree), CLOG_SUCCESS);

    EXPECT_EQ(clog_queue_enqueue(nullptr, 123), CLOG_INVALID_PARAM);
    EXPECT_EQ(clog_queue_dequeue(nullptr, nullptr), CLOG_INVALID_PARAM);
    EXPECT_EQ(clog_queue_peek(nullptr, nullptr), CLOG_INVALID_PARAM);

    EXPECT_EQ(clog_queue_dequeue(queue, nullptr), CLOG_INVALID_PARAM);
    EXPECT_EQ(clog_queue_peek(queue, nullptr), CLOG_INVALID_PARAM);

    EXPECT_TRUE(clog_queue_is_empty(nullptr));
    EXPECT_EQ(clog_queue_size(nullptr), 0u);

    clog_queue_clear(nullptr);
    clog_queue_destroy(nullptr);
}
