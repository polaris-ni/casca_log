/**
 * @auther Polaris
 * @date  2025/10/8
 */
#include <gtest/gtest.h>
#include "casca_log.h"
#include "clog_error.h"
#include "clog_hooks.h"
#include "clog_recorder_manager.h"
#include "clog_config.h"
#include "clog_secure_func.h"

TEST(CascaLogTest, CreateLog)
{
    FILE* fp = fopen("../../casca_log_config_template.toml", "r");
    char buffer[1024] = {0};
    ASSERT_NE(fp, nullptr);
    CLOG_IGNORE_RES(fseek(fp, 0, SEEK_END));
    const long len = ftell(fp);
    const auto buf = static_cast<char*>(clog_malloc(len + 1));
    ASSERT_NE(buf, nullptr);
    CLOG_IGNORE_RES(clog_memset(buf, len + 1, 0, len + 1));
    CLOG_IGNORE_RES(fseek(fp, 0, SEEK_SET));
    CLOG_IGNORE_RES(fread(buf, 1, len, fp));
    CLOG_IGNORE_RES(fclose(fp));

    clog_err_setup(16, 128);
    clog_res_e ret = clog_init("casca_log_test", buf);
    if (ret != CLOG_SUCCESS) {
        clog_err_print(buffer, sizeof(buffer), nullptr);
        printf("reason: \n%s\n", buffer);
    }
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_free(buf);

    ret = clog_setup(nullptr, 0);
    if (ret != CLOG_SUCCESS) {
        clog_err_print(buffer, sizeof(buffer), nullptr);
        printf("reason: \n%s\n", buffer);
    }
    ASSERT_EQ(ret, CLOG_SUCCESS);

    const auto tmp = static_cast<char*>(clog_malloc(len));
    ASSERT_NE(buf, nullptr);
    const clog_config_group_t* root = clog_get_config_root();
    clog_config_dump_group(root, tmp, len, "  ");
    printf("config dump:\n%s\n==================================================\n", tmp);

    ret = clog_log("test", CLOG_RECORDER_STDOUT_ID, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_TRACE,
                   "test log print process trace [%s]", clog_get_process());
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_log("test", CLOG_RECORDER_STDOUT_ID, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_DEBUG,
                   "test log print process debug [%s]", clog_get_process());
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_log("test", CLOG_RECORDER_STDOUT_ID, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_INFO,
                   "test log print process info [%s]", clog_get_process());
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_log("test", CLOG_RECORDER_STDOUT_ID, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_WARN,
                   "test log print process warn [%s]", clog_get_process());
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_log("test", CLOG_RECORDER_STDOUT_ID, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_ERROR,
                   "test log print process error [%s]", clog_get_process());
    ASSERT_EQ(ret, CLOG_SUCCESS);
    ret = clog_log("test", CLOG_RECORDER_STDOUT_ID, CLOG_FILENAME, __FUNCTION__, __LINE__, CLOG_LEVEL_FETAL,
                   "test log print process fetal [%s]", clog_get_process());
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_free(tmp);
    clog_destroy(nullptr, 0);
    clog_err_cleanup();
}
