/**
 * @author Polaris
 * @date 2026/3/6
 */

#include <gtest/gtest.h>
#include "casca_log_core.h"
#include "clog_channel_atomic_queue.h"
#include "clog_dispatcher_async_thread.h"
#include "clog_error.h"
#include "clog_recorder_manager.h"
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
};

const char *CLogCoreTest::process = "casca_log_test";

TEST_F(CLogCoreTest, BasicLogTest)
{
    clog_context_t *context;
    clog_res_e ret = clog_context_create(process, &context);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_set_mask(context, CLOG_LEVEL_ALL);
    ret = clog_set_buffer_pool(context, true, 128, 75);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_module_t module = {};
    CLOG_IGNORE_RES(clog_strcpy(module.name, sizeof(module.name), "test"));
    module.level = CLOG_LEVEL_ALL;
    module.num = 1;
    module.recorders[0] = CLOG_RECORDER_ID_STDOUT;
    ret = clog_add_module(context, &module);
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
    clog_context_destroy(&context);
}
