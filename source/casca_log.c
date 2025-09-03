/**
 * @auther Polaris
 * @date  2025/9/3
 */

#include "casca_log.h"
#include "casca_log_hooks.h"

clog_context_t* clog_create(void)
{
    clog_context_t* ctx = clog_malloc(sizeof(clog_context_t));
    if (ctx == NULL) {
        return NULL;
    }
    ctx->level = CLOG_LEVEL_DEBUG;
    return ctx;
}
