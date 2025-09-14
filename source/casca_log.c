/**
 * @auther Polaris
 * @date  2025/9/3
 */

#include "casca_log.h"

#include <stdarg.h>
#include <stdio.h>

#include "clog_hooks.h"
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

void clog_err_clear(clog_context_t* context)
{
    CLOG_RET_VOID_IF_NULL(context);
    context->err[0] = '\0';
}

void clog_err_set(clog_context_t* context, const char* fmt, ...)
{
    CLOG_RET_VOID_IF_NULL(context);
    CLOG_RET_VOID_IF(fmt == NULL);
    va_list args;
    va_start(args, fmt);
    const int ret = vsnprintf(context->err, sizeof(context->err), fmt, args);
    va_end(args);
    if (ret <= 0) {
        context->err[0] = '\0';
    }
}

const char* clog_err_get(const clog_context_t* context)
{
    if (context == NULL) {
        return "NULL";
    }
    return context->err;
}
