/**
 * @author Polaris
 * @date  2025/11/13
 */
#include <algorithm>
#include <chrono>
#include <gtest/gtest.h>
#include "clog_atomic_queue.h"
#include "test_util.h"

class CLogAtomicQueueTest : public ::testing::Test
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
};

TEST_F(CLogAtomicQueueTest, CreateAndDestroy)
{
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));

    ASSERT_NE(nullptr, queue);
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_destroy(queue));
    queue = nullptr;
}

TEST_F(CLogAtomicQueueTest, CreateWithNullPointer)
{
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_create(nullptr, 32, 50));
}

TEST_F(CLogAtomicQueueTest, EnqueueNormal)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 64, 50));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    constexpr int data = 12345;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, &data, sizeof(data)));
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, EnqueueWithNullQueue)
{
    constexpr int data = 12345;
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_enqueue(nullptr, &data, sizeof(data)));
}

TEST_F(CLogAtomicQueueTest, EnqueueWithNullData)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 64, 50));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_enqueue(handle, nullptr, sizeof(int)));
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, MultipleEnqueue)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, &i, sizeof(i)));
    }
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, DequeueFromEmptyQueue)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    int data;
    size_t len;
    EXPECT_EQ(CLOG_TARGET_NOT_FOUND, clog_atomic_queue_dequeue(handle, &data, sizeof(data), &len));
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, EnqueueAndDequeue)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    constexpr int data_in = 54321;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, &data_in, sizeof(data_in)));

    int data_out;
    size_t len;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_dequeue(handle, &data_out, sizeof(data_out), &len));
    EXPECT_EQ(data_in, data_out);
    EXPECT_EQ(sizeof(data_in), len);
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, DequeueWithSmallBuffer)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 40, 50));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    const int data_in[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, data_in, sizeof(data_in)));

    int data_out[5];
    size_t len;
    EXPECT_EQ(CLOG_OVERSIZE, clog_atomic_queue_dequeue(handle, data_out, sizeof(data_out), &len));
    EXPECT_EQ(sizeof(data_out), len);
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, DequeueWithoutLength)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    int data_in = 999;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, &data_in, sizeof(data_in)));

    int data_out;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_dequeue(handle, &data_out, sizeof(data_out), nullptr));
    EXPECT_EQ(data_in, data_out);
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, DequeueToNullBuffer)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    constexpr int data_in = 111;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, &data_in, sizeof(data_in)));

    size_t len;
    EXPECT_EQ(CLOG_OVERSIZE, clog_atomic_queue_dequeue(handle, nullptr, 0, &len));
    EXPECT_EQ(sizeof(data_in), len);
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, IsEmpty)
{
    EXPECT_TRUE(clog_atomic_queue_is_empty(nullptr));

    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 32, 50));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);
    EXPECT_TRUE(clog_atomic_queue_is_empty(handle));

    constexpr int data = 1;
    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, &data, sizeof(data)));
    EXPECT_FALSE(clog_atomic_queue_is_empty(handle));

    EXPECT_EQ(CLOG_OVERSIZE, clog_atomic_queue_dequeue(handle, nullptr, 0, nullptr));
    EXPECT_TRUE(clog_atomic_queue_is_empty(handle));
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, DestroyNullQueue)
{
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_atomic_queue_destroy(nullptr));
}

TEST_F(CLogAtomicQueueTest, LargeDataOperations)
{
    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, 1024, 50));
    clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
    EXPECT_NE(handle, nullptr);

    constexpr int count = 1000;
    for (int i = 0; i < count; ++i) {
        EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, &i, sizeof(i))) << "Failed at iteration " << i;
    }

    for (int i = 0; i < count; ++i) {
        int data_out;
        size_t len;
        EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_dequeue(handle, &data_out, sizeof(data_out), &len));
        EXPECT_EQ(i, data_out);
        EXPECT_EQ(sizeof(i), len);
    }

    EXPECT_TRUE(clog_atomic_queue_is_empty(handle));
    clog_atomic_queue_detach(handle);
}

TEST_F(CLogAtomicQueueTest, MultiProducerSingleConsumer)
{
    constexpr size_t item_size = sizeof(size_t);
    constexpr size_t num_producers = 4;
    constexpr size_t num_items_per_producer = 1000;

    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, item_size, 50));

    std::vector<std::thread> producers;
    std::atomic start_flag{false};
    std::atomic total_produced{0};

    producers.reserve(num_producers);
    for (size_t i = 0; i < num_producers; ++i) {
        producers.emplace_back(
            [this, i, &start_flag, &total_produced, num_items_per_producer]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }

                clog_atomic_queue_handle_t handle =
                    clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_ENQUEUE);
                ASSERT_NE(handle, nullptr);

                for (size_t j = 0; j < num_items_per_producer; ++j) {
                    size_t value = i * num_items_per_producer + j;
                    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, &value, sizeof(value)));
                    total_produced.fetch_add(1);
                }

                clog_atomic_queue_detach(handle);
            });
    }

    std::thread consumer(
        [this, &start_flag, num_producers, num_items_per_producer]
        {
            start_flag.store(true);

            clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_DEQUEUE);
            ASSERT_NE(handle, nullptr);

            std::vector received(num_producers * num_items_per_producer, false);
            int items_received = 0;

            while (items_received < num_producers * num_items_per_producer) {
                size_t value;
                size_t len;
                const clog_res_e res = clog_atomic_queue_dequeue(handle, &value, sizeof(value), &len);
                if (res == CLOG_SUCCESS) {
                    ASSERT_LT(value, num_producers * num_items_per_producer);
                    ASSERT_FALSE(received[value]);
                    received[value] = true;
                    items_received++;
                } else if (res != CLOG_TARGET_NOT_FOUND) {
                    FAIL() << "Unexpected dequeue result: " << res;
                }

                if (res == CLOG_TARGET_NOT_FOUND) {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }

            for (size_t i = 0; i < num_producers * num_items_per_producer; ++i) {
                ASSERT_TRUE(received[i]) << "Missing Item: " << i;
            }

            clog_atomic_queue_detach(handle);
        });

    for (auto& producer : producers) {
        producer.join();
    }
    consumer.join();
}

TEST_F(CLogAtomicQueueTest, SingleProducerMultiConsumer)
{
    constexpr size_t item_size = sizeof(size_t);
    constexpr size_t num_consumers = 4;
    constexpr size_t num_items = 1000;

    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, item_size, 50));

    std::atomic start_flag{false};
    std::atomic total_consumed{0};
    std::vector<std::thread> consumers;
    std::vector<size_t> consumed_values;
    std::mutex consumed_values_mutex;

    consumers.reserve(num_consumers);
    for (size_t i = 0; i < num_consumers; ++i) {
        consumers.emplace_back(
            [this, &start_flag, &total_consumed, &consumed_values, &consumed_values_mutex, num_items]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }

                clog_atomic_queue_handle_t handle =
                    clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_DEQUEUE);
                ASSERT_NE(handle, nullptr);

                while (total_consumed.load() < num_items) {
                    size_t value;
                    size_t len;
                    const clog_res_e res = clog_atomic_queue_dequeue(handle, &value, sizeof(value), &len);
                    if (res == CLOG_SUCCESS) {
                        total_consumed.fetch_add(1);
                        {
                            std::lock_guard lock(consumed_values_mutex);
                            consumed_values.push_back(value);
                        }
                    } else if (res != CLOG_TARGET_NOT_FOUND) {
                        FAIL() << "Unexpected dequeue result: " << res;
                    }

                    if (res == CLOG_TARGET_NOT_FOUND) {
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                }

                clog_atomic_queue_detach(handle);
            });
    }

    std::thread producer(
        [this, &start_flag, num_items]
        {
            clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_ENQUEUE);
            ASSERT_NE(handle, nullptr);

            start_flag.store(true);

            for (int i = 0; i < num_items; ++i) {
                EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, &i, sizeof(i)));
            }

            clog_atomic_queue_detach(handle);
        });

    producer.join();
    for (auto& consumer : consumers) {
        consumer.join();
    }

    EXPECT_EQ(total_consumed.load(), num_items);
    EXPECT_EQ(consumed_values.size(), num_items);

    std::sort(consumed_values.begin(), consumed_values.end());
    const auto last = std::unique(consumed_values.begin(), consumed_values.end());
    EXPECT_EQ(last, consumed_values.end()) << "Duplicate values found";
}

TEST_F(CLogAtomicQueueTest, MultiProducerMultiConsumer)
{
    constexpr size_t item_size = sizeof(size_t);
    constexpr size_t num_producers = 3;
    constexpr size_t num_consumers = 3;
    constexpr size_t num_items_per_producer = 500;

    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, item_size, 50));

    std::atomic start_flag{false};
    std::atomic total_produced{0};
    std::atomic total_consumed{0};
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::vector<size_t> consumed_values;
    std::mutex consumed_values_mutex;

    producers.reserve(num_producers);
    for (size_t i = 0; i < num_producers; ++i) {
        producers.emplace_back(
            [this, i, &start_flag, &total_produced, num_items_per_producer]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }

                clog_atomic_queue_handle_t handle =
                    clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_ENQUEUE);
                ASSERT_NE(handle, nullptr);

                for (size_t j = 0; j < num_items_per_producer; ++j) {
                    size_t value = i * num_items_per_producer + j;
                    EXPECT_EQ(CLOG_SUCCESS, clog_atomic_queue_enqueue(handle, &value, sizeof(value)));
                    total_produced.fetch_add(1);
                }

                clog_atomic_queue_detach(handle);
            });
    }

    constexpr size_t items_to_consume = num_producers * num_items_per_producer / num_consumers + 10;

    consumers.reserve(num_consumers);
    for (size_t i = 0; i < num_consumers; ++i) {
        constexpr size_t total_num = num_producers * num_items_per_producer;
        consumers.emplace_back(
            [this, &start_flag, &total_consumed, &consumed_values, &consumed_values_mutex, items_to_consume, total_num]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }

                clog_atomic_queue_handle_t handle =
                    clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_DEQUEUE);
                ASSERT_NE(handle, nullptr);

                for (size_t j = 0; j < items_to_consume && total_consumed.load() < total_num;) {
                    size_t value;
                    size_t len;
                    const clog_res_e res = clog_atomic_queue_dequeue(handle, &value, sizeof(value), &len);
                    if (res == CLOG_SUCCESS) {
                        total_consumed.fetch_add(1);
                        {
                            std::lock_guard lock(consumed_values_mutex);
                            consumed_values.push_back(value);
                        }
                        j++;
                    } else if (res != CLOG_TARGET_NOT_FOUND) {
                        FAIL() << "Unexpected dequeue result: " << res;
                    }

                    if (res == CLOG_TARGET_NOT_FOUND) {
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

    EXPECT_EQ(total_produced.load(), num_producers * num_items_per_producer);
    EXPECT_EQ(total_consumed.load(), num_producers * num_items_per_producer);
    EXPECT_EQ(consumed_values.size(), num_producers * num_items_per_producer);

    std::sort(consumed_values.begin(), consumed_values.end());
    const auto last = std::unique(consumed_values.begin(), consumed_values.end());
    EXPECT_EQ(last, consumed_values.end()) << "Duplicate values found";

    for (size_t i = 0; i < num_producers * num_items_per_producer; ++i) {
        bool found = std::binary_search(consumed_values.begin(), consumed_values.end(), i);
        EXPECT_TRUE(found) << "Missing value: " << i;
    }
}

TEST_F(CLogAtomicQueueTest, MultiThreadsWithDifferentBiases)
{
    constexpr size_t item_size = sizeof(size_t);
    constexpr size_t num_threads = 4;

    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, item_size, 50));

    std::vector<std::thread> threads;
    std::atomic start_flag{false};
    std::atomic total_operations{0};

    clog_atomic_queue_bias_e biases[] = {CLOG_ATOMIC_QUEUE_BIASED_NONE, CLOG_ATOMIC_QUEUE_BIASED_ENQUEUE,
                                         CLOG_ATOMIC_QUEUE_BIASED_DEQUEUE, CLOG_ATOMIC_QUEUE_BIASED_ANY};

    for (size_t i = 0; i < num_threads; ++i) {
        constexpr size_t num_items_per_thread = 500;
        threads.emplace_back(
            [this, i, &start_flag, &total_operations, biases, num_items_per_thread]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }

                clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(this->queue, biases[i % 4]);
                ASSERT_NE(handle, nullptr);
                clog_res_e res;
                for (size_t j = 0; j < num_items_per_thread; ++j) {
                    if (j % 2 == 0) {
                        size_t value = i * num_items_per_thread + j;
                        res = clog_atomic_queue_enqueue(handle, &value, sizeof(value));
                        if (res == CLOG_SUCCESS) {
                            total_operations.fetch_add(1);
                        }
                    } else {
                        int value;
                        size_t len;
                        res = clog_atomic_queue_dequeue(handle, &value, sizeof(value), &len);
                        if (res == CLOG_SUCCESS || res == CLOG_OVERSIZE) {
                            total_operations.fetch_add(1);
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
    ASSERT_NE(handle, nullptr);

    size_t remaining_count = 0;
    size_t value;
    size_t len;
    while (clog_atomic_queue_dequeue(handle, &value, sizeof(value), &len) == CLOG_SUCCESS) {
        remaining_count++;
    }

    clog_atomic_queue_detach(handle);

    EXPECT_GT(total_operations.load(), 0);
}

TEST_F(CLogAtomicQueueTest, HighConcurrencyPerformance)
{
    constexpr size_t item_size = sizeof(size_t);
    constexpr size_t num_threads = 8;
    constexpr size_t num_items_per_thread = 1000;

    ASSERT_EQ(CLOG_SUCCESS, clog_atomic_queue_create(&queue, item_size, 50));

    std::vector<std::thread> threads;
    std::atomic start_flag{false};
    std::atomic<size_t> sum_of_squared_values{0};

    const auto start_time = std::chrono::high_resolution_clock::now();

    threads.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(
            [this, i, &start_flag, &sum_of_squared_values, &num_items_per_thread]
            {
                while (!start_flag.load()) {
                    std::this_thread::yield();
                }

                clog_atomic_queue_handle_t handle = clog_atomic_queue_attach(this->queue, CLOG_ATOMIC_QUEUE_BIASED_ANY);
                ASSERT_NE(handle, nullptr);

                for (size_t j = 0; j < num_items_per_thread; ++j) {
                    if (j % 2 == 0) {
                        size_t value = i * num_items_per_thread + j;
                        const clog_res_e res = clog_atomic_queue_enqueue(handle, &value, sizeof(value));
                        if (res != CLOG_SUCCESS) {
                            FAIL() << "Enqueue failed with result: " << res;
                        }
                    } else {
                        size_t value;
                        size_t len;
                        const clog_res_e res = clog_atomic_queue_dequeue(handle, &value, sizeof(value), &len);
                        if (res == CLOG_SUCCESS) {
                            sum_of_squared_values.fetch_add(value * value);
                        } else if (res != CLOG_TARGET_NOT_FOUND && res != CLOG_OVERSIZE) {
                            FAIL() << "Dequeue failed with unexpected result: " << res;
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
    ASSERT_NE(cleanup_handle, nullptr);

    size_t value;
    size_t len;
    while (clog_atomic_queue_dequeue(cleanup_handle, &value, sizeof(value), &len) == CLOG_SUCCESS) {
        sum_of_squared_values.fetch_add(value * value);
    }

    clog_atomic_queue_detach(cleanup_handle);

    EXPECT_GT(sum_of_squared_values.load(), 0);
}
