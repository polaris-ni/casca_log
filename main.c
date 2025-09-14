#include <stdio.h>
#include "casca_log.h"
#include "casca_log_defines.h"
#include "utils/clog_config.h"
#include "utils/clog_mem_pool.h"
#include "utils/clog_secure_func.h"

int main(void)
{
    clog_context_t* ctx = clog_create("casca_log_test");
    CLOG_RET_IF_NULL(ctx, CLOG_FAIL);
    printf("process = %s, level = %u!\n", ctx->process, ctx->config.level);
    FILE* fp = fopen("../casca_log_test.toml", "r");
    CLOG_RET_IF_NULL(fp, CLOG_FAIL);
    fseek(fp, 0, SEEK_END);
    const long len = ftell(fp);
    char* buf = clog_malloc(len + 1);
    clog_memset(buf, len + 1, 0, len + 1);
    CLOG_RET_IF_NULL(buf, CLOG_FAIL);
    fseek(fp, 0, SEEK_SET);
    fread(buf, 1, len, fp);
    fclose(fp);
    buf[len] = '\0';
    printf("\n==============================\n%s\n==============================\n", buf);
    const clog_res_e ret = clog_config_load(ctx, buf);
    if (ret != CLOG_SUCCESS) {
        printf("%s", clog_err_get(ctx));
        clog_destroy(ctx);
        return ret;
    }
    char* tmp = clog_malloc(len);
    CLOG_RET_IF_NULL(tmp, CLOG_FAIL);
    clog_config_dump(ctx, tmp, len);
    printf("config dump:\n%s", tmp);
    clog_destroy(ctx);
    return CLOG_SUCCESS;
}
