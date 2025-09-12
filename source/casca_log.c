/**
 * @auther Polaris
 * @date  2025/9/3
 */

#include "casca_log.h"
#include "clog_mem_pool.h"
#include "utils/clog_secure_func.h"

clog_context_t* clog_create(const char *process)
{
    clog_context_t* ctx = clog_malloc(sizeof(clog_context_t));
    if (ctx == NULL) {
        return NULL;
    }
    (void)clog_memset(ctx, sizeof(clog_context_t), 0, sizeof(clog_context_t));
    ctx->process = clog_str_dup(process);
    if (process == NULL) {
        clog_free(ctx);
        return NULL;
    }
#ifdef CASCA_LOG_MEM_POOL
    clog_mp_init(CLOG_MP_PRE_ALLOCATED_NORMAL);
#endif
    return ctx;
}

void clog_destroy(clog_context_t* context)
{
    (void)clog_memset(context, sizeof(clog_context_t), 0, sizeof(clog_context_t));
    clog_free(context);
}
