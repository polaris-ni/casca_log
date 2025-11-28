/**
 * @author Polaris
 * @date  2025/11/29
 */
#include <chrono>
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <vector>
#include "clog_atomic_mpsc_queue.h"
#include "test_util.h"

class CLogAtomicMpscQueueTest : public ::testing::Test
{
protected:
    clog_atomic_mpsc_queue_t* queue = nullptr;

    void SetUp() override
    {
        queue = clog_atomic_mpsc_queue_create();
        ASSERT_NE(nullptr, queue);
    }

    void TearDown() override
    {
        if (queue != nullptr) {
            clog_atomic_mpsc_atomic_destroy(queue);
            queue = nullptr;
        }
    }
};

TEST_F(CLogAtomicMpscQueueTest, EmptyQueueReturnsNotFound)
{
    uintptr_t data;
    const clog_res_e result = clog_atomic_mpsc_queue_out(queue, &data);
    EXPECT_EQ(CLOG_TARGET_NOT_FOUND, result);
}

TEST_F(CLogAtomicMpscQueueTest, SingleThreadEnqueueDequeue)
{
    constexpr uintptr_t test_data = 42;
    clog_res_e result = clog_atomic_mpsc_queue_in(queue, test_data);
    EXPECT_EQ(CLOG_SUCCESS, result);
    uintptr_t data;
    result = clog_atomic_mpsc_queue_out(queue, &data);
    EXPECT_EQ(CLOG_SUCCESS, result);
    EXPECT_EQ(test_data, data);
    result = clog_atomic_mpsc_queue_out(queue, &data);
    EXPECT_EQ(CLOG_TARGET_NOT_FOUND, result);
}

TEST_F(CLogAtomicMpscQueueTest, MultiProducerSingleConsumer)
{
    constexpr size_t NUM_PRODUCERS = 4;
    constexpr size_t NUM_ITEMS_PER_PRODUCER = 1000;
    constexpr size_t TOTAL_ITEMS = NUM_PRODUCERS * NUM_ITEMS_PER_PRODUCER;

    CLogTest::CLogProducerConsumerDataHolder<NUM_PRODUCERS, 1, uintptr_t> holder(NUM_ITEMS_PER_PRODUCER, TOTAL_ITEMS);

    std::vector<std::thread> producers;
    std::atomic<bool> stop_consumer{false};
    std::atomic<size_t> consumed_count{0};

    auto consumer = std::thread(
        [&]
        {
            while (!stop_consumer.load() || consumed_count.load() < TOTAL_ITEMS) {
                uintptr_t data;
                const clog_res_e result = clog_atomic_mpsc_queue_out(queue, &data);
                if (result == CLOG_SUCCESS) {
                    holder.Consume(0, data);
                    consumed_count.fetch_add(1);
                } else {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }
        });

    for (size_t i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(
            [&, i]()
            {
                std::mt19937 rng(static_cast<unsigned>(i + std::time(nullptr)));
                std::uniform_int_distribution<uintptr_t> dist(1, 1000000);

                for (size_t j = 0; j < NUM_ITEMS_PER_PRODUCER; ++j) {
                    uintptr_t value = dist(rng);
                    holder.Produce(i, value);

                    clog_res_e result = clog_atomic_mpsc_queue_in(queue, value);
                    EXPECT_EQ(CLOG_SUCCESS, result);

                    if (j % 100 == 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                }
            });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop_consumer.store(true);
    consumer.join();
    holder.Validate(TOTAL_ITEMS);
}

TEST_F(CLogAtomicMpscQueueTest, HighConcurrencyStressTest)
{
    constexpr size_t NUM_PRODUCERS = 16;
    constexpr size_t NUM_ITEMS_PER_PRODUCER = 1000;
    constexpr size_t TOTAL_ITEMS = NUM_PRODUCERS * NUM_ITEMS_PER_PRODUCER;

    std::vector<std::thread> producers;
    std::atomic<size_t> consumed_count{0};
    CLogTest::CLogProducerConsumerDataHolder<NUM_PRODUCERS, 1, uintptr_t> holder(NUM_ITEMS_PER_PRODUCER, TOTAL_ITEMS);
    auto consumer = std::thread(
        [&]
        {
            while (consumed_count.load() < TOTAL_ITEMS) {
                uintptr_t data;
                const clog_res_e result = clog_atomic_mpsc_queue_out(queue, &data);
                if (result == CLOG_SUCCESS) {
                    holder.Consume(0, data);
                    consumed_count.fetch_add(1);
                } else {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }
        });

    producers.reserve(NUM_PRODUCERS);
    for (size_t i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(
            [&, i]
            {
                for (size_t j = 0; j < NUM_ITEMS_PER_PRODUCER; ++j) {
                    const uintptr_t value = (i * NUM_ITEMS_PER_PRODUCER) + j;
                    clog_res_e result = clog_atomic_mpsc_queue_in(queue, value);
                    EXPECT_EQ(CLOG_SUCCESS, result);
                    holder.Produce(i, value);
                }
            });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    consumer.join();

    EXPECT_EQ(TOTAL_ITEMS, consumed_count.load());
    holder.Validate(TOTAL_ITEMS);
}

TEST_F(CLogAtomicMpscQueueTest, RapidEnqueueDequeue)
{
    constexpr size_t NUM_OPERATIONS = 10000;

    std::thread producer(
        [&]
        {
            for (size_t i = 0; i < NUM_OPERATIONS; ++i) {
                clog_res_e result = clog_atomic_mpsc_queue_in(queue, i);
                EXPECT_EQ(CLOG_SUCCESS, result);
            }
        });

    std::thread consumer(
        [&]
        {
            for (size_t i = 0; i < NUM_OPERATIONS; ++i) {
                uintptr_t data;
                clog_res_e result;
                do {
                    result = clog_atomic_mpsc_queue_out(queue, &data);
                    if (result == CLOG_TARGET_NOT_FOUND) {
                        std::this_thread::yield();
                    }
                } while (result == CLOG_TARGET_NOT_FOUND);

                EXPECT_EQ(CLOG_SUCCESS, result);
                EXPECT_EQ(i, data);
            }
        });

    producer.join();
    consumer.join();
}
