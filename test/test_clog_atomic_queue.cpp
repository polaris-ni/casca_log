/**
 * @author Polaris
 * @date  2025/11/13
 */
#include <gtest/gtest.h>
#include "clog_atomic_queue.h"

class AtomicQueueTest : public ::testing::Test
{
protected:
    clog_atomic_queue_t* queue = nullptr;

    void SetUp() override
    {
        queue = nullptr;
    }

    void TearDown() override
    {
        if (queue) {
            clog_atomic_queue_destroy(queue);
            queue = nullptr;
        }
    }
};

TEST_F(AtomicQueueTest, CreateAndDestroy)
{
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));

    ASSERT_NE(nullptr, queue);
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_destroy(queue));
    queue = nullptr;
}

TEST_F(AtomicQueueTest, CreateWithNullPointer)
{
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_create(nullptr, 32, 50));
}

TEST_F(AtomicQueueTest, EnqueueNormal)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 64, 50));

    constexpr int data = 12345;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(queue, &data, sizeof(data)));
}

TEST_F(AtomicQueueTest, EnqueueWithNullQueue)
{
    constexpr int data = 12345;
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_enqueue(nullptr, &data, sizeof(data)));
}

TEST_F(AtomicQueueTest, EnqueueWithNullData)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 64, 50));
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_enqueue(queue, nullptr, sizeof(int)));
}

TEST_F(AtomicQueueTest, MultipleEnqueue)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(queue, &i, sizeof(i)));
    }
}

TEST_F(AtomicQueueTest, DequeueFromEmptyQueue)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));

    int data;
    size_t len;
    EXPECT_EQ(CLOG_TARGET_NOT_FOUND, clog_atomic_queue_dequeue(queue, &data, sizeof(data), &len));
}

TEST_F(AtomicQueueTest, EnqueueAndDequeue)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));

    int data_in = 54321;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(queue, &data_in, sizeof(data_in)));

    int data_out;
    size_t len;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_dequeue(queue, &data_out, sizeof(data_out), &len));
    EXPECT_EQ(data_in, data_out);
    EXPECT_EQ(sizeof(data_in), len);
}

TEST_F(AtomicQueueTest, DequeueWithSmallBuffer)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 40, 50));

    const int data_in[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(queue, data_in, sizeof(data_in)));

    int data_out[5];
    size_t len;
    EXPECT_EQ(CLOG_OVERSIZE, clog_atomic_queue_dequeue(queue, data_out, sizeof(data_out), &len));
    EXPECT_EQ(sizeof(data_out), len);
}

TEST_F(AtomicQueueTest, DequeueWithoutLength)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));

    int data_in = 999;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(queue, &data_in, sizeof(data_in)));

    int data_out;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_dequeue(queue, &data_out, sizeof(data_out), nullptr));
    EXPECT_EQ(data_in, data_out);
}

TEST_F(AtomicQueueTest, DequeueToNullBuffer)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));

    constexpr int data_in = 111;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(queue, &data_in, sizeof(data_in)));

    size_t len;
    EXPECT_EQ(CLOG_OVERSIZE, clog_atomic_queue_dequeue(queue, nullptr, 0, &len));
    EXPECT_EQ(sizeof(data_in), len);
}

TEST_F(AtomicQueueTest, IsEmpty)
{
    EXPECT_TRUE(clog_atomic_queue_is_empty(nullptr));

    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));
    EXPECT_TRUE(clog_atomic_queue_is_empty(queue));

    constexpr int data = 1;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(queue, &data, sizeof(data)));
    EXPECT_FALSE(clog_atomic_queue_is_empty(queue));

    EXPECT_EQ(CLOG_OVERSIZE, clog_atomic_queue_dequeue(queue, nullptr, 0, nullptr));
    EXPECT_TRUE(clog_atomic_queue_is_empty(queue));
}

TEST_F(AtomicQueueTest, DestroyNullQueue)
{
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_destroy(nullptr));
}

TEST_F(AtomicQueueTest, LargeDataOperations)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 1024, 50));

    constexpr int count = 1000;
    for (int i = 0; i < count; ++i) {
        EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(queue, &i, sizeof(i))) << "Failed at iteration " << i;
    }

    for (int i = 0; i < count; ++i) {
        int data_out;
        size_t len;
        EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_dequeue(queue, &data_out, sizeof(data_out), &len));
        EXPECT_EQ(i, data_out);
        EXPECT_EQ(sizeof(i), len);
    }

    EXPECT_TRUE(clog_atomic_queue_is_empty(queue));
}
