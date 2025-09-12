#include <stdio.h>
#include "casca_log.h"
#include "casca_log_defines.h"
#include "utils/clog_mem_pool.h"
#include "utils/clog_secure_func.h"

int main(void)
{
    clog_context_t* ctx = clog_create("casca_log_test");
    CLOG_RET_IF_NULL(ctx, CLOG_FAIL);
    printf("process = %s, level = %u!\n", ctx->process, ctx->config.level);
    char* str = clog_mp_allocate(10);
    clog_memset(str, 11, 'a', 11);
    if (str != NULL) {
        str[11] = '\0';
    }
    printf("%s\n", str);
    clog_destroy(ctx);
    return CLOG_SUCCESS;
}
