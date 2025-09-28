#include <stdio.h>
#include "casca_log.h"
#include "casca_log_defines.h"
#include "utils/clog_config.h"
#include "utils/clog_secure_func.h"

int main(void)
{
    FILE* fp = fopen("../casca_log_config_template.toml", "r");
    CLOG_RET_IF_NULL(fp, CLOG_FAIL);
    fseek(fp, 0, SEEK_END);
    const long len = ftell(fp);
    char* buf = clog_malloc(len + 1);
    if (buf == NULL) {
        (void)fclose(fp);
        return CLOG_FAIL;
    }
    (void)clog_memset(buf, len + 1, 0, len + 1);
    (void)fseek(fp, 0, SEEK_SET);
    (void)fread(buf, 1, len, fp);
    (void)fclose(fp);
    fp = NULL;

    const clog_res_e ret = clog_init("casca_log_test", buf);
    if (ret != CLOG_SUCCESS) {
        clog_free(buf);
        printf("create context failed, reason: %s", clog_err_get());
        return ret;
    }
    clog_free(buf);
    char* tmp = clog_malloc(len);
    if (tmp == NULL) {
        clog_destroy();
        return CLOG_FAIL;
    }
    const clog_config_group_t* root = clog_get_config_root();
    clog_config_dump_group(root, tmp, len);
    printf("config dump:\n%s", tmp);
    clog_free(tmp);
    clog_destroy();
    clog_err_clear();
    return CLOG_SUCCESS;
}
