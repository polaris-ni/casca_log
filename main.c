#include <stdio.h>
#include "casca_log.h"

int main(void)
{
    clog_context_t* ctx = clog_create();
    CLOG_RET_IF_NULL(ctx, CLOG_FAIL);
    printf("level = %u!\n", ctx->level);
    clog_destroy(ctx);
    return CLOG_SUCCESS;
}
