/**
 * @auther Polaris
 * @date  2025/10/8
 */
#include <gtest/gtest.h>
#include "casca_log.h"
#include "clog_hooks.h"
#include "utils/clog_config.h"
#include "utils/clog_secure_func.h"

TEST(CascaLogTest, CreateLog)
{
    FILE* fp = fopen("../../casca_log_config_template.toml", "r");
    ASSERT_NE(fp, nullptr);
    fseek(fp, 0, SEEK_END);
    const long len = ftell(fp);
    const auto buf = static_cast<char*>(clog_malloc(len));
    ASSERT_NE(buf, nullptr);
    CLOG_IGNORE_RES(clog_memset(buf, len + 1, 0, len + 1));
    CLOG_IGNORE_RES(fseek(fp, 0, SEEK_SET));
    CLOG_IGNORE_RES(fread(buf, 1, len, fp));
    CLOG_IGNORE_RES(fclose(fp));

    clog_res_e ret = clog_init("casca_log_test", buf);
    if (ret != CLOG_SUCCESS) {
        printf("reason: \n\t%s\n", clog_err_get());
    }
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_free(buf);

    ret = clog_setup(nullptr, 0);
    ASSERT_EQ(ret, CLOG_SUCCESS);

    const auto tmp = static_cast<char*>(clog_malloc(len));
    ASSERT_NE(buf, nullptr);
    const clog_config_group_t* root = clog_get_config_root();
    clog_config_dump_group(root, tmp, len);
    printf("config dump:\n%s\n==================================================\n", tmp);

    const uint32_t recorder = 1;
    ret = clog_log(&recorder, 1, "test", __FILE_NAME__, __FUNCTION__, __LINE__, CLOG_LEVEL_INFO, "test log print process > %s", clog_get_process());
    ASSERT_EQ(ret, CLOG_SUCCESS);
    clog_free(tmp);
    clog_destroy(nullptr, 0);
    clog_err_clear();
}
