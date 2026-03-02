/**
 * @auther Polaris
 * @date  2025/10/8
 */
#include <gtest/gtest.h>
#include "casca_log.h"
#include "clog_config.h"
#include "clog_error.h"
#include "clog_hooks.h"
#include "clog_recorder_manager.h"
#include "clog_secure_func.h"
#include "clog_thread.h"
#include "test_util.h"

class CLogMainTest : public testing::Test
{
protected:
    void SetUp() override
    {
        CLogTest::CLogMemLeakDetect::start(nullptr, nullptr);
    }

    void TearDown() override
    {
        CLogTest::CLogMemLeakDetect::end();
    }
};

TEST_F(CLogMainTest, CreateLog)
{
    FILE *fp = fopen("../../casca_log_config_template.toml", "rb");
    char buffer[1024] = {};
    EXPECT_NE(fp, nullptr);
    EXPECT_EQ(fseek(fp, 0, SEEK_END), 0);
    const long len = ftell(fp);
    const auto buf = static_cast<char *>(clog_malloc(len + 1));
    EXPECT_NE(buf, nullptr);
    EXPECT_EQ(clog_memset(buf, len + 1, 0, len + 1), CLOG_SUCCESS);
    EXPECT_EQ(fseek(fp, 0, SEEK_SET), 0);
    EXPECT_EQ(fread(buf, 1, len, fp), len);
    EXPECT_EQ(fclose(fp), 0);

    clog_err_setup(32, 256);
    clog_res_e ret = clog_init("casca_log_test", buf);
    if (ret != CLOG_SUCCESS) {
        clog_err_print(buffer, sizeof(buffer), nullptr);
        GTEST_LOG_(ERROR) << "reason: \n" << buffer << std::endl;
    }
    EXPECT_EQ(ret, CLOG_SUCCESS);
    clog_free(buf);

    ret = clog_setup(nullptr, 0);
    if (ret != CLOG_SUCCESS) {
        clog_err_print(buffer, sizeof(buffer), nullptr);
        GTEST_LOG_(ERROR) << "reason: \n" << buffer << std::endl;
    }
    EXPECT_EQ(ret, CLOG_SUCCESS);

    const auto tmp = static_cast<char *>(clog_malloc(len));
    EXPECT_NE(buf, nullptr);
    const clog_config_group_t *root = clog_get_config_root();
    EXPECT_TRUE(clog_config_dump_group(root, tmp, len, "  "));

    ret = clog_log("test", CLOG_RECORDER_ID_STDOUT, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_TRACE,
                   "test log print process trace [%s]", clog_get_process());
    EXPECT_EQ(ret, CLOG_SUCCESS);
    ret = clog_log("test", CLOG_RECORDER_ID_STDOUT, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_DEBUG,
                   "test log print process debug [%s]", clog_get_process());
    EXPECT_EQ(ret, CLOG_SUCCESS);
    ret = clog_log("test", CLOG_RECORDER_ID_STDOUT, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_INFO,
                   "test log print process info [%s]", clog_get_process());
    EXPECT_EQ(ret, CLOG_SUCCESS);
    ret = clog_log("test", CLOG_RECORDER_ID_STDOUT, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_WARN,
                   "test log print process warn [%s]", clog_get_process());
    EXPECT_EQ(ret, CLOG_SUCCESS);
    ret = clog_log("test", CLOG_RECORDER_ID_STDOUT, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_ERROR,
                   "test log print process error [%s]", clog_get_process());
    EXPECT_EQ(ret, CLOG_SUCCESS);
    ret = clog_log("test", CLOG_RECORDER_ID_STDOUT, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_FETAL,
                   "test log print process fetal [%s]", clog_get_process());
    EXPECT_EQ(ret, CLOG_SUCCESS);
    ret = clog_log("test", CLOG_RECORDER_ID_FILE, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_FETAL,
                   "test log print process to file fetal [%s]", clog_get_process());
    EXPECT_EQ(ret, CLOG_SUCCESS);
    clog_free(tmp);
    clog_thread_sleep(1000);
    clog_destroy(nullptr, 0);
    clog_err_cleanup();
}
