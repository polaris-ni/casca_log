#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "clog_ebr.h"

static std::atomic<int> freed_count(0);
void test_deallocator(void* ptr)
{
    freed_count.fetch_add(1);
    clog_free(ptr);
}

class CLogEbrTest : public ::testing::Test
{
protected:
    clog_ebr_global_t* global = nullptr;

    void SetUp() override
    {
        EXPECT_EQ(clog_ebr_create(&global, test_deallocator), CLOG_SUCCESS);
        freed_count.store(0);
    }

    void TearDown() override
    {
        if (global == nullptr) {
            return;
        }
        int result;
        do {
            result = clog_ebr_destroy(global);
            if (result == CLOG_NOT_COMPLETED) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        } while (result == CLOG_NOT_COMPLETED);
        EXPECT_EQ(result, CLOG_SUCCESS);
        global = nullptr;
    }
};

TEST_F(CLogEbrTest, BasicCreateAndState)
{
    EXPECT_NE(global, nullptr);
    EXPECT_EQ(clog_ebr_get_global_state(global), CLOG_EBR_GLOBAL_STATE_ACTIVE_NORMAL);
}

TEST_F(CLogEbrTest, SingleThreadRegisterUnregister)
{
    clog_ebr_thread_local_t* local;
    EXPECT_EQ(clog_ebr_register(global, &local), CLOG_SUCCESS);
    EXPECT_NE(local, nullptr);
    EXPECT_EQ(clog_ebr_get_local_state(local), CLOG_EBR_LOCAL_STATE_INACTIVE);

    clog_ebr_unregister(local);
}

TEST_F(CLogEbrTest, SingleThreadEnterExit)
{
    clog_ebr_thread_local_t* local;
    EXPECT_EQ(clog_ebr_register(global, &local), CLOG_SUCCESS);

    EXPECT_EQ(clog_ebr_enter(local), CLOG_SUCCESS);
    EXPECT_EQ(clog_ebr_get_local_state(local), CLOG_EBR_LOCAL_STATE_ACTIVE);

    clog_ebr_exit(local);
    EXPECT_EQ(clog_ebr_get_local_state(local), CLOG_EBR_LOCAL_STATE_INACTIVE);

    clog_ebr_unregister(local);
}

TEST_F(CLogEbrTest, SingleThreadDeferRelease)
{
    clog_ebr_thread_local_t* local;
    EXPECT_EQ(clog_ebr_register(global, &local), CLOG_SUCCESS);

    void* ptr = malloc(10);
    EXPECT_EQ(clog_ebr_enter(local), CLOG_SUCCESS);
    EXPECT_NE(ptr, nullptr);
    clog_ebr_exit(local);
    clog_ebr_defer_release(local, ptr);

    clog_ebr_poll(global);
    clog_ebr_poll(global);
    clog_ebr_poll(global);
    EXPECT_GT(freed_count.load(), 0);

    clog_ebr_unregister(local);
}

TEST_F(CLogEbrTest, MultiThreadRegister)
{
    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<clog_ebr_thread_local_t*> locals(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &locals]
                             { EXPECT_EQ(clog_ebr_register(this->global, &locals[i]), CLOG_SUCCESS); });
    }

    for (auto& t : threads) {
        t.join();
    }

    for (int i = 0; i < num_threads; ++i) {
        EXPECT_NE(locals[i], nullptr);
        clog_ebr_unregister(locals[i]);
    }
}

TEST_F(CLogEbrTest, MultiThreadEnterExit)
{
    clog_ebr_thread_local_t* local;
    EXPECT_EQ(clog_ebr_register(global, &local), CLOG_SUCCESS);

    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> active_count(0);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(
            [local, &active_count]
            {
                for (int j = 0; j < 100; ++j) {
                    if (clog_ebr_enter(local) == CLOG_SUCCESS) {
                        active_count.fetch_add(1);
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                        active_count.fetch_sub(1);
                        clog_ebr_exit(local);
                    }
                }
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(active_count.load(), 0);
    clog_ebr_unregister(local);
}

TEST_F(CLogEbrTest, MultiThreadDeferRelease)
{
    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<clog_ebr_thread_local_t*> locals(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        EXPECT_EQ(clog_ebr_register(global, &locals[i]), CLOG_SUCCESS);
    }

    std::atomic<int> total_allocated(0);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(
            [this, i, &locals, &total_allocated]
            {
                for (int j = 0; j < 100; ++j) {
                    EXPECT_EQ(clog_ebr_enter(locals[i]), CLOG_SUCCESS);
                    void* ptr = clog_malloc(10);
                    ASSERT_NE(ptr, nullptr);
                    total_allocated.fetch_add(1);
                    clog_ebr_exit(locals[i]);
                    clog_ebr_defer_release(locals[i], ptr);

                    if (j % 10 == 0) {
                        clog_ebr_poll(this->global);
                    }
                }
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    clog_ebr_poll(global);

    for (int i = 0; i < num_threads; ++i) {
        clog_ebr_unregister(locals[i]);
    }
}

TEST_F(CLogEbrTest, ErrorHandling)
{
    clog_ebr_thread_local_t* local;

    EXPECT_EQ(clog_ebr_register(nullptr, &local), CLOG_INVALID_PARAM);
    EXPECT_EQ(clog_ebr_register(global, nullptr), CLOG_INVALID_PARAM);

    EXPECT_EQ(clog_ebr_register(global, &local), CLOG_SUCCESS);

    clog_ebr_thread_local_t* local2;
    EXPECT_EQ(clog_ebr_register(global, &local2), CLOG_SUCCESS);

    clog_ebr_destroy(global);
    EXPECT_EQ(clog_ebr_enter(local), CLOG_ABNORMAL_STATE);

    clog_ebr_unregister(local);
    clog_ebr_unregister(local2);
    global = nullptr;
}
