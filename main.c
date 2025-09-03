#include <stdio.h>
#include "casca_log.h"

int main(void)
{
    const clog_context_t* ctx = clog_create();
    if (ctx == NULL) {
        return CLOG_FAIL;
    }
    printf("level = %u!\n", ctx->level);
    return CLOG_SUCCESS;
}
