/**
 * @author Polaris
 * @date 2026/3/6
 */

#include <chrono>
#include <gtest/gtest.h>
#include "casca_log_core.h"
#include "clog_channel_atomic_queue.h"
#include "clog_channel_unsafe_queue.h"
#include "clog_dispatcher_async_thread.h"
#include "clog_filter_keywords.h"
#include "clog_recorder_file.h"
#include "clog_recorder_stdout.h"
#include "clog_recorder_syslog.h"
#include "clog_thread.h"
#include "test_util.h"

class CLogCoreTest : public testing::Test
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

    static clog_res_e clog_recorder_test_write(clog_recorder_t *recorder, const clog_item_t *item)
    {
        CLOG_UNUSED_VAR(item);
        auto *count = static_cast<size_t *>(recorder->extra);
        *count = *count + 1;
        return CLOG_SUCCESS;
    }
};

const char *CLogCoreTest::process = "casca_log_test";

TEST_F(CLogCoreTest, AtomicQueueLogTest)
{
    clog_context_t *context;
    clog_res_e ret = clog_context_create(process, &context);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_set_mask(context, CLOG_LEVEL_ALL);
    ret = clog_set_buffer_pool(context, true, 128, 75);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_module_t module = {
        "test",
        CLOG_LEVEL_ALL,
        3,
        {CLOG_RECORDER_ID_STDOUT, CLOG_RECORDER_ID_FILE, CLOG_RECORDER_ID_SYSLOG},
    };
    ret = clog_add_module(context, &module);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_add_filter(context, clog_filter_keywords_create(1, "password"));
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_set_log_format(context,
                              "{_year}-{_month}-{_day} {_hour}:{_minute}:{_second}.{_millisecond} {_level} "
                              "[{_process}.{_module}:{_tid} {_file}:{_line}] {_content}{_ln}");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_channel_t *channel = clog_atomic_queue_channel_create();
    ASSERT_NE(channel, nullptr);
    ret = clog_set_channel(context, channel);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_dispatcher_t *dispatcher = clog_dispatcher_async_thread_create();
    ret = clog_set_dispatcher(context, dispatcher);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_recorder_stdout_attr_t stdout_attr = {
        CLOG_RECORDER_STDOUT_COLOR_TRUE,
        {
            {CLOG_COLOR_OF(187, 187, 187), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(0, 112, 187), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(72, 187, 49), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(243, 156, 17), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(255, 0, 0), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(255, 255, 255), CLOG_COLOR_OF(139, 0, 0)},
        },
    };
    ret = clog_add_recorder(context, clog_recorder_stdout_create(&stdout_attr));
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_recorder_file_attr_t file_attr = {
        "{_cwd}{_path_separator}logs{_path_separator}metainfo",
        "{_cwd}{_path_separator}logs{_path_separator}{_year}{_month}{_day}{_path_separator}",
        "log_{_log_index}_{_year}{_month}{_day}_{_hour}{_minute}{_second}.log",
        {CLOG_RECORDER_FILE_SPLIT_SIZE, 1048576},
    };
    ret = clog_add_recorder(context, clog_recorder_file_create(&file_attr));
    ASSERT_EQ(ret, CLOG_SUCCESS);
#ifdef CLOG_PLATFORM_LINUX
    ret = clog_add_recorder(context, clog_recorder_syslog_create(process, LOG_PID | LOG_ODELAY, LOG_USER));
    ASSERT_EQ(ret, CLOG_SUCCESS);
#endif
    ret = clog_context_setup(context);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_TRACE, "test trace log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_DEBUG, "test debug log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_INFO, "test info log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_WARN, "test warn log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_ERROR, "test error log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_FETAL, "test fetal log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_INFO, "filter password");
    ASSERT_EQ(ret, CLOG_NOT_PERMITTED);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_INFO, "hello, %s!", "world");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_context_destroy(&context);
}

TEST_F(CLogCoreTest, UnsafeQueueLogTest)
{
    clog_context_t *context;
    clog_res_e ret = clog_context_create(process, &context);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_set_mask(context, CLOG_LEVEL_ALL);
    ret = clog_set_buffer_pool(context, true, 128, 75);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_module_t module = {"test", CLOG_LEVEL_ALL, 2, {CLOG_RECORDER_ID_STDOUT, CLOG_RECORDER_ID_FILE}};
    ret = clog_add_module(context, &module);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_add_filter(context, clog_filter_keywords_create(1, "password"));
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_set_log_format(context,
                              "{_year}-{_month}-{_day} {_hour}:{_minute}:{_second}.{_millisecond} {_level} "
                              "[{_process}.{_module}:{_tid} {_file}:{_line}] {_content}{_ln}");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_channel_t *channel = clog_unsafe_queue_channel_create();
    ASSERT_NE(channel, nullptr);
    ret = clog_set_channel(context, channel);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_dispatcher_t *dispatcher = clog_dispatcher_async_thread_create();
    ret = clog_set_dispatcher(context, dispatcher);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_recorder_stdout_attr_t stdout_attr = {
        CLOG_RECORDER_STDOUT_COLOR_TRUE,
        {
            {CLOG_COLOR_OF(187, 187, 187), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(0, 112, 187), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(72, 187, 49), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(243, 156, 17), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(255, 0, 0), CLOG_COLOR_UNSPECIFIED},
            {CLOG_COLOR_OF(255, 255, 255), CLOG_COLOR_OF(139, 0, 0)},
        },
    };
    ret = clog_add_recorder(context, clog_recorder_stdout_create(&stdout_attr));
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_recorder_file_attr_t file_attr = {
        "{_cwd}{_path_separator}logs{_path_separator}metainfo",
        "{_cwd}{_path_separator}logs{_path_separator}{_year}{_month}{_day}{_path_separator}",
        "log_{_log_index}_{_year}{_month}{_day}_{_hour}{_minute}{_second}.log",
        {CLOG_RECORDER_FILE_SPLIT_SIZE, 1048576},
    };
    ret = clog_add_recorder(context, clog_recorder_file_create(&file_attr));
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_context_setup(context);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_TRACE, "test trace log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_DEBUG, "test debug log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_INFO, "test info log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_WARN, "test warn log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_ERROR, "test error log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_FETAL, "test fetal log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_INFO, "filter password");
    ASSERT_EQ(ret, CLOG_NOT_PERMITTED);
    ret = CLOG_MODULE_LOG(context, module.name, CLOG_LEVEL_INFO, "hello, %s!", "world");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_context_destroy(&context);
}

TEST_F(CLogCoreTest, MultiThreadLogTest)
{
    clog_context_t *context;
    clog_res_e ret = clog_context_create(process, &context);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_set_mask(context, CLOG_LEVEL_ALL);
    ret = clog_set_buffer_pool(context, true, 128, 75);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_module_t module = {"test", CLOG_LEVEL_ALL, 2, {CLOG_RECORDER_ID_RESERVED + 1}};
    ret = clog_add_module(context, &module);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_add_filter(context, clog_filter_keywords_create(1, "password"));
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_set_log_format(context,
                              "{_year}-{_month}-{_day} {_hour}:{_minute}:{_second}.{_millisecond} {_level} "
                              "[{_process}.{_module}:{_tid} {_file}:{_line}] {_content}{_ln}");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_channel_t *channel = clog_atomic_queue_channel_create();
    ASSERT_NE(channel, nullptr);
    ret = clog_set_channel(context, channel);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_dispatcher_t *dispatcher = clog_dispatcher_async_thread_create();
    ret = clog_set_dispatcher(context, dispatcher);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    auto *recorder = static_cast<clog_recorder_t *>(clog_malloc(sizeof(clog_recorder_t)));
    ASSERT_NE(recorder, nullptr);
    recorder->id = CLOG_RECORDER_ID_RESERVED + 1;
    recorder->open = clog_recorder_empty_open;
    recorder->write = clog_recorder_test_write;
    recorder->flush = clog_recorder_empty_flush;
    recorder->close = clog_recorder_empty_close;
    auto *count = static_cast<size_t *>(clog_malloc(sizeof(size_t)));
    ASSERT_NE(count, nullptr);
    *count = 0;
    recorder->extra = count;
    ret = clog_add_recorder(context, recorder);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_context_setup(context);
    ASSERT_EQ(ret, CLOG_SUCCESS);

    constexpr size_t NUM_PRODUCERS = 10;
    constexpr size_t NUM_ITEMS_PER_PRODUCER = 10000;
    std::vector<std::thread> producers;
    producers.reserve(NUM_PRODUCERS);
    auto startTime = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(
            [&]
            {
                for (size_t j = 0; j < NUM_ITEMS_PER_PRODUCER; ++j) {
                    constexpr clog_level_e levels[] = {
                        CLOG_LEVEL_TRACE, CLOG_LEVEL_DEBUG, CLOG_LEVEL_INFO,
                        CLOG_LEVEL_WARN,  CLOG_LEVEL_ERROR, CLOG_LEVEL_FETAL,
                    };
                    const clog_level_e level = levels[j % CLOG_LEVEL_NUM];
                    const clog_res_e res = CLOG_MODULE_LOG(context, module.name, level, "test trace log");
                    ASSERT_EQ(res, CLOG_SUCCESS);
                }
            });
    }
    for (auto &producer : producers) {
        producer.join();
    }
    clog_context_destroy(&context);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "time cost: " << duration.count() << "ms" << std::endl;
    ASSERT_EQ(*count, NUM_PRODUCERS * NUM_ITEMS_PER_PRODUCER);
    clog_free(count);
}
