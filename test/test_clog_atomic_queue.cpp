/**
 * @author Polaris
 * @date  2025/11/13
 */
#include <algorithm>
#include <chrono>
#include <gtest/gtest.h>
#include "clog_atomic_queue.h"
#include "test_util.h"

class CLogAtomicQueueTest : public testing::Test
{
protected:
    clog_atomic_queue_t* queue = nullptr;

    void SetUp() override
    {
        queue = nullptr;
        CLogTest::CLogMemLeakDetect::start(nullptr, nullptr);
    }

    void TearDown() override
    {
        if (queue != nullptr) {
            clog_atomic_queue_destroy(queue);
            queue = nullptr;
        }
        CLogTest::CLogMemLeakDetect::end();
    }

    static void CLogAtomicQueueTestNoFree(void* ptr)
    {
        CLOG_UNUSED_VAR(ptr);
    }
};

TEST_F(CLogAtomicQueueTest, CreateAndDestroy)
{
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));

    ASSERT_NE(nullptr, queue);
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_destroy(queue));
    queue = nullptr;
}

TEST_F(CLogAtomicQueueTest, CreateWithNullPointer)
{
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_create(nullptr, CLogAtomicQueueTestNoFree));
}

TEST_F(CLogAtomicQueueTest, EnqueueNormal)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    constexpr std::uintptr_t data = 12345;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, data));
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, EnqueueWithNullQueue)
{
    constexpr std::uintptr_t data = 12345;
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_enqueue(nullptr, data));
}

TEST_F(CLogAtomicQueueTest, MultipleEnqueue)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    for (std::size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, i));
    }
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, DequeueFromEmptyQueue)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    std::uintptr_t data;
    EXPECT_EQ(CLOG_TARGET_NOT_FOUND, clog_atomic_queue_dequeue(handle, &data));
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, EnqueueAndDequeue)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    constexpr std::uintptr_t data_in = 54321;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, data_in));

    std::uintptr_t data_out;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_dequeue(handle, &data_out));
    EXPECT_EQ(data_in, data_out);
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, IsEmpty)
{
    EXPECT_TRUE(clog_atomic_queue_is_empty(nullptr));

    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);
    EXPECT_TRUE(clog_atomic_queue_is_empty(handle));

    constexpr std::uintptr_t data = 1;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, data));
    EXPECT_FALSE(clog_atomic_queue_is_empty(handle));

    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_dequeue(handle, nullptr));
    std::uintptr_t data_out;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_dequeue(handle, &data_out));
    EXPECT_TRUE(clog_atomic_queue_is_empty(handle));
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, DestroyNullQueue)
{
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_destroy(nullptr));
}

TEST_F(CLogAtomicQueueTest, LargeDataOperations)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    constexpr std::size_t count = 1000;
    for (std::size_t i = 0; i < count; ++i) {
        EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, i)) << "Failed at iteration " << i;
    }

    for (std::size_t i = 0; i < count; ++i) {
        std::uintptr_t data_out;
        EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_dequeue(handle, &data_out));
        EXPECT_EQ(i, data_out);
    }

    EXPECT_TRUE(clog_atomic_queue_is_empty(handle));
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, MultiProducerSingleConsumer)
{
    CLogTest::CLogProducerConsumerDataHolder<4, 1, std::uintptr_t> holder(1000, 0);

    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));

    std::vector<std::thread> producers;
    std::atomic start_flag{false};

    producers.reserve(holder.producer_num);
    for (std::size_t i = 0; i < holder.producer_num; ++i) {
        producers.emplace_back(
            [this, i, &start_flag, &holder]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }

                clog_atomic_queue_handle_t handle =
                    clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_ENQUEUE);
                EXPECT_NE(handle, nullptr);

                for (std::size_t j = 0; j < holder.num_per_producer; ++j) {
                    const std::uintptr_t value = i * holder.num_per_producer + j;
                    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, value));
                    holder.Produce(i, value);
                }

                clog_atomic_queue_detach(handle);
            });
    }

    std::thread consumer(
        [this, &start_flag, &holder]
        {
            start_flag.store(true);

            clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_DEQUEUE);
            EXPECT_NE(handle, nullptr);
            std::size_t items_received = 0;

            while (items_received < holder.producer_num * holder.num_per_producer) {
                std::uintptr_t value;
                const clog_res_e res = clog_atomic_queue_dequeue(handle, &value);
                if (res == CLOG_SUCCESS) {
                    holder.Consume(0, value);
                    ++items_received;
                } else if (res != CLOG_TARGET_NOT_FOUND) {
                    FAIL() << "Unexpected dequeue result: " << res;
                } else {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }

            clog_atomic_queue_detach(handle);
        });

    for (auto& producer : producers) {
        producer.join();
    }
    consumer.join();
    holder.Validate(holder.producer_num * holder.num_per_producer);
}

TEST_F(CLogAtomicQueueTest, SingleProducerMultiConsumer)
{
    CLogTest::CLogProducerConsumerDataHolder<1, 4, std::uintptr_t> holder(0, 1000);

    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));

    std::atomic start_flag{false};
    std::atomic total_consumed{0};
    std::vector<std::thread> consumers;
    std::vector<std::size_t> consumed_values;

    consumers.reserve(holder.consumer_num);
    for (std::size_t i = 0; i < holder.consumer_num; ++i) {
        consumers.emplace_back(
            [this, &start_flag, &total_consumed, &holder, i]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }

                clog_atomic_queue_handle_t handle =
                    clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_DEQUEUE);
                EXPECT_NE(handle, nullptr);

                while (total_consumed.load() < holder.consumer_num * holder.num_per_consumer) {
                    std::uintptr_t value;
                    const clog_res_e res = clog_atomic_queue_dequeue(handle, &value);
                    if (res == CLOG_SUCCESS) {
                        holder.Consume(i, value);
                        total_consumed.fetch_add(1);
                    } else if (res != CLOG_TARGET_NOT_FOUND) {
                        FAIL() << "Unexpected dequeue result: " << res;
                    } else {
                    }
                }

                clog_atomic_queue_detach(handle);
            });
    }

    std::thread producer(
        [this, &start_flag, &holder]
        {
            clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_ENQUEUE);
            EXPECT_NE(handle, nullptr);

            start_flag.store(true);

            for (std::size_t i = 0; i < holder.consumer_num * holder.num_per_consumer; ++i) {
                EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, i));
                holder.Produce(0, i);
            }

            clog_atomic_queue_detach(handle);
        });

    producer.join();
    for (auto& consumer : consumers) {
        consumer.join();
    }

    EXPECT_EQ(total_consumed.load(), holder.consumer_num * holder.num_per_consumer);
    holder.Validate(holder.consumer_num * holder.num_per_consumer);
}

TEST_F(CLogAtomicQueueTest, MultiProducerMultiConsumer)
{

    CLogTest::CLogProducerConsumerDataHolder<8, 8, std::uintptr_t> holder(1000, 1000);

    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));

    std::atomic start_flag{false};
    std::atomic total_produced{0};
    std::atomic total_consumed{0};
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    producers.reserve(holder.producer_num);
    for (std::size_t i = 0; i < holder.producer_num; ++i) {
        producers.emplace_back(
            [this, i, &start_flag, &total_produced, &holder]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }

                clog_atomic_queue_handle_t handle =
                    clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_ENQUEUE);
                EXPECT_NE(handle, nullptr);

                for (std::size_t j = 0; j < holder.num_per_producer; ++j) {
                    std::uintptr_t value = i * holder.num_per_producer + j;
                    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, value));
                    holder.Produce(i, value);
                    total_produced.fetch_add(1);
                }

                clog_atomic_queue_detach(handle);
            });
    }

    consumers.reserve(holder.consumer_num);
    for (std::size_t i = 0; i < holder.consumer_num; ++i) {

        consumers.emplace_back(
            [this, &start_flag, &total_consumed, &holder, i]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }
                const std::size_t total_num = holder.producer_num * holder.num_per_producer;
                const std::size_t consume_num = total_num / holder.consumer_num + 10;
                clog_atomic_queue_handle_t handle =
                    clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_DEQUEUE);
                EXPECT_NE(handle, nullptr);

                for (std::size_t j = 0; j < consume_num && total_consumed.load() < total_num;) {
                    std::uintptr_t value;
                    const clog_res_e res = clog_atomic_queue_dequeue(handle, &value);
                    if (res == CLOG_SUCCESS) {
                        total_consumed.fetch_add(1);
                        holder.Consume(i, value);
                        j++;
                    } else if (res != CLOG_TARGET_NOT_FOUND) {
                        FAIL() << "Unexpected dequeue result: " << res;
                    } else {
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                }

                clog_atomic_queue_detach(handle);
            });
    }

    start_flag.store(true);

    for (auto& producer : producers) {
        producer.join();
    }
    for (auto& consumer : consumers) {
        consumer.join();
    }

    EXPECT_EQ(total_produced.load(), holder.producer_num * holder.num_per_producer);
    EXPECT_EQ(total_consumed.load(), total_consumed.load());
    holder.Validate(total_produced.load());
}

TEST_F(CLogAtomicQueueTest, MultiThreadsWithDifferentBiases)
{
    constexpr std::size_t num_threads = 4;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));

    std::vector<std::thread> threads;
    std::atomic start_flag{false};
    std::atomic total_operations{0};

    clog_atomic_queue_bias_e biases[] = {CLOG_ATOMIC_QUEUE_BIASED_NONE, CLOG_ATOMIC_QUEUE_BIASED_ENQUEUE,
                                         CLOG_ATOMIC_QUEUE_BIASED_DEQUEUE, CLOG_ATOMIC_QUEUE_BIASED_ANY};

    for (std::size_t i = 0; i < num_threads; ++i) {
        std::size_t num_items_per_thread = 500;
        threads.emplace_back(
            [this, i, &start_flag, &total_operations, biases, num_items_per_thread]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }

                clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(this->queue, biases[i % 4]);
                EXPECT_NE(handle, nullptr);
                clog_res_e res;
                for (std::size_t j = 0; j < num_items_per_thread; ++j) {
                    if (j % 2 == 0) {
                        const std::uintptr_t value = i * num_items_per_thread + j;
                        res = clog_atomic_queue_enqueue(handle, value);
                        if (res == CLOG_SUCCESS) {
                            total_operations.fetch_add(1);
                        }
                    } else {
                        std::uintptr_t value;
                        res = clog_atomic_queue_dequeue(handle, &value);
                        if (res == CLOG_SUCCESS) {
                            total_operations.fetch_add(1);
                        } else {
                            std::cout << "Unexpected dequeue result: " << res << "\n";
                        }
                    }
                }

                clog_atomic_queue_detach(handle);
            });
    }

    start_flag.store(true);

    for (auto& thread : threads) {
        thread.join();
    }

    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    std::size_t remaining_count = 0;
    std::uintptr_t value;
    while (clog_atomic_queue_dequeue(handle, &value) == CLOG_SUCCESS) {
        remaining_count++;
    }

    clog_atomic_queue_detach(handle);
    EXPECT_GT(total_operations.load(), 0);
}

TEST_F(CLogAtomicQueueTest, HighConcurrencyPerformance)
{
    constexpr std::size_t num_threads = 16;
    std::size_t num_items_per_thread = 10000;

    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, CLogAtomicQueueTestNoFree));

    std::vector<std::thread> threads;
    std::atomic start_flag{false};
    std::atomic<std::size_t> sum_of_squared_values{0};

    const auto start_time = std::chrono::high_resolution_clock::now();

    threads.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(
            [this, i, &start_flag, &sum_of_squared_values, &num_items_per_thread]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }

                clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
                EXPECT_NE(handle, nullptr);

                for (std::size_t j = 0; j < num_items_per_thread; ++j) {
                    if (j % 2 == 0) {
                        const std::uintptr_t value = i * num_items_per_thread + j;
                        const clog_res_e res = clog_atomic_queue_enqueue(handle, value);
                        if (res != CLOG_SUCCESS) {
                            FAIL() << "Enqueue failed with result: " << res;
                        }
                    } else {
                        std::uintptr_t value;
                        const clog_res_e res = clog_atomic_queue_dequeue(handle, &value);
                        if (res == CLOG_SUCCESS) {
                            sum_of_squared_values.fetch_add(value * value);
                        } else if (res != CLOG_TARGET_NOT_FOUND) {
                            FAIL() << "Dequeue failed with unexpected result: " << res;
                        } else {
                            std::this_thread::sleep_for(std::chrono::microseconds(1));
                        }
                    }
                }

                clog_atomic_queue_detach(handle);
            });
    }

    start_flag.store(true);

    for (auto& thread : threads) {
        thread.join();
    }

    const auto end_time = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    GTEST_LOG_(INFO) << "High concurrency test completed in " << duration.count() << " ms with " << num_threads
                     << " threads and " << num_items_per_thread << " items per thread";

    clog_atomic_queue_handle_t cleanup_handle = clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(cleanup_handle, nullptr);

    std::uintptr_t value;
    while (clog_atomic_queue_dequeue(cleanup_handle, &value) == CLOG_SUCCESS) {
        sum_of_squared_values.fetch_add(value * value);
    }
    clog_atomic_queue_detach(cleanup_handle);
    EXPECT_GT(sum_of_squared_values.load(), 0);
}
