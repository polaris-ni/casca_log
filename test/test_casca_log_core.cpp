/**
 * @author Polaris
 * @date 2026/3/6
 */

#include <gtest/gtest.h>
#include "casca_log_core.h"
#include "clog_channel_atomic_queue.h"
#include "clog_dispatcher_async_thread.h"
#include "clog_filter_keywords.h"
#include "clog_recorder_file.h"
#include "clog_recorder_stdout.h"
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
    module.num = 2;
    module.recorders[0] = CLOG_RECORDER_ID_STDOUT;
    module.recorders[1] = CLOG_RECORDER_ID_FILE;
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
    clog_recorder_stdout_attr_t stdout_attr;
    stdout_attr.mode = CLOG_RECORDER_STDOUT_COLOR_TRUE;
    stdout_attr.colors[CLOG_LEVEL_TRACE - 1].background.enabled = false;
    stdout_attr.colors[CLOG_LEVEL_TRACE - 1].foreground.enabled = true;
    stdout_attr.colors[CLOG_LEVEL_TRACE - 1].foreground.value = CLOG_COLOR_OF(128, 128, 128);
    stdout_attr.colors[CLOG_LEVEL_DEBUG - 1].background.enabled = false;
    stdout_attr.colors[CLOG_LEVEL_DEBUG - 1].foreground.enabled = true;
    stdout_attr.colors[CLOG_LEVEL_DEBUG - 1].foreground.value = CLOG_COLOR_OF(160, 160, 192);
    stdout_attr.colors[CLOG_LEVEL_INFO - 1].background.enabled = false;
    stdout_attr.colors[CLOG_LEVEL_INFO - 1].foreground.enabled = true;
    stdout_attr.colors[CLOG_LEVEL_INFO - 1].foreground.value = CLOG_COLOR_OF(255, 255, 255);
    stdout_attr.colors[CLOG_LEVEL_WARN - 1].background.enabled = false;
    stdout_attr.colors[CLOG_LEVEL_WARN - 1].foreground.enabled = true;
    stdout_attr.colors[CLOG_LEVEL_WARN - 1].foreground.value = CLOG_COLOR_OF(255, 255, 0);
    stdout_attr.colors[CLOG_LEVEL_ERROR - 1].background.enabled = false;
    stdout_attr.colors[CLOG_LEVEL_ERROR - 1].foreground.enabled = true;
    stdout_attr.colors[CLOG_LEVEL_ERROR - 1].foreground.value = CLOG_COLOR_OF(255, 255, 0);
    stdout_attr.colors[CLOG_LEVEL_FETAL - 1].foreground.enabled = true;
    stdout_attr.colors[CLOG_LEVEL_FETAL - 1].foreground.value = CLOG_COLOR_OF(255, 255, 255);
    stdout_attr.colors[CLOG_LEVEL_FETAL - 1].background.enabled = true;
    stdout_attr.colors[CLOG_LEVEL_FETAL - 1].background.value = CLOG_COLOR_OF(139, 0, 0);
    ret = clog_add_recorder(context, clog_recorder_stdout_create(&stdout_attr));
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_recorder_file_attr_t file_attr;
    file_attr.metainfo_path = "{_cwd}{_path_separator}logs{_path_separator}metainfo";
    file_attr.log_dir = "{_cwd}{_path_separator}logs{_path_separator}{_year}{_month}{_day}{_path_separator}";
    file_attr.log_name = "log_{_log_index}_{_year}{_month}{_day}_{_hour}{_minute}{_second}.log";
    file_attr.split.type = CLOG_RECORDER_FILE_SPLIT_SIZE;
    file_attr.split.limit = 1048576;
    ret = clog_add_recorder(context, clog_recorder_file_create(&file_attr));
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_context_setup(context);
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_module_log(context, module.name, CLOG_FILENAME, __func__, __LINE__, CLOG_LEVEL_TRACE, "test trace log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_module_log(context, module.name, CLOG_FILENAME, __func__, __LINE__, CLOG_LEVEL_DEBUG, "test debug log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_module_log(context, module.name, CLOG_FILENAME, __func__, __LINE__, CLOG_LEVEL_INFO, "test info log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_module_log(context, module.name, CLOG_FILENAME, __func__, __LINE__, CLOG_LEVEL_WARN, "test warn log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_module_log(context, module.name, CLOG_FILENAME, __func__, __LINE__, CLOG_LEVEL_ERROR, "test error log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_module_log(context, module.name, CLOG_FILENAME, __func__, __LINE__, CLOG_LEVEL_FETAL, "test fetal log");
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_module_log(context, module.name, CLOG_FILENAME, __func__, __LINE__, CLOG_LEVEL_INFO, "filter password");
    ASSERT_EQ(ret, CLOG_NOT_PERMITTED);
    clog_thread_sleep(1000);
    clog_context_destroy(&context);
}
