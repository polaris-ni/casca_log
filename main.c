#include <stdio.h>
#include "casca_log.h"
#include "casca_log_defines.h"
#include "utils/clog_config.h"
#include "utils/clog_mem_pool.h"
#include "utils/clog_secure_func.h"

int main(void)
{
    FILE* fp = fopen("../casca_log_test.toml", "r");
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

    char err[512] = {0};
    clog_context_t* ctx = clog_create("casca_log_test", buf, err, sizeof(err));
    if (ctx == NULL) {
        clog_free(buf);
        printf("create context failed, reason: %s", err);
        return CLOG_FAIL;
    }
    clog_free(buf);
    char* tmp = clog_malloc(len);
    if (tmp == NULL) {
        clog_destroy(ctx);
        return CLOG_FAIL;
    }
    clog_config_dump_group(ctx->config.raw, tmp, len);
    printf("config dump:\n%s", tmp);
    clog_free(tmp);
    clog_destroy(ctx);
    return CLOG_SUCCESS;
}
