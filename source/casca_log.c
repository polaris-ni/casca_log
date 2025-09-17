/**
 * @author Polaris
 * @date  2025/9/3
 */

#include "casca_log.h"

#include <stdarg.h>
#include <stdio.h>

#include "clog_config.h"
#include "clog_hooks.h"
#include "clog_mem_pool.h"
#include "utils/clog_secure_func.h"

clog_context_t* clog_create(const char* process, const char* config, char* err, const size_t size)
{
    CLOG_RET_IF_NULL(process, NULL);
    CLOG_RET_IF_NULL(config, NULL);
    clog_context_t* ctx = clog_malloc(sizeof(clog_context_t));
    if (ctx == NULL) {
        return NULL;
    }
    (void)clog_memset(ctx, sizeof(clog_context_t), 0, sizeof(clog_context_t));
    ctx->process = clog_strdup(process);
    if (ctx->process == NULL) {
        clog_free(ctx);
        return NULL;
    }
    ctx->config.raw = clog_config_parse(config, err, size);
    if (ctx->config.raw == NULL) {
        clog_destroy(ctx);
        return NULL;
    }
#ifdef CASCA_LOG_MEM_POOL
    clog_mp_init(CLOG_MP_PRE_ALLOCATED_NORMAL);
#endif
    return ctx;
}

void clog_destroy(clog_context_t* context)
{
    CLOG_RET_VOID_IF_NULL(context);
    CLOG_FREE_IF_NOT_NULL(context->process);
    clog_config_destroy_group(context->config.raw);
    context->config.level = CLOG_LEVEL_OFF;
    CLOG_FREE_IF_NOT_NULL(context);
}

void clog_err_clear(clog_context_t* context)
{
    CLOG_RET_VOID_IF_NULL(context);
    context->err[0] = '\0';
}

void clog_err_set(clog_context_t* context, const char* fmt, ...)
{
    CLOG_RET_VOID_IF_NULL(context);
    CLOG_RET_VOID_IF_NULL(fmt);
    va_list args;
    va_start(args, fmt);
    const int ret = vsnprintf(context->err, sizeof(context->err), fmt, args);
    va_end(args);
    if (ret <= 0) {
        context->err[0] = '\0';
    }
}

void clog_err_append(clog_context_t* context, const char* fmt, ...)
{
    CLOG_RET_VOID_IF_NULL(context);
    CLOG_RET_VOID_IF_NULL(fmt);
    const size_t len = strlen(context->err);
    CLOG_RET_VOID_IF(len >= sizeof(context->err) - 1);
    va_list args;
    va_start(args, fmt);
    const int ret = vsnprintf(context->err + len, sizeof(context->err) - len, fmt, args);
    va_end(args);
    if (ret <= 0) {
        context->err[0] = '\0';
    }
}

void clog_err_append_line(clog_context_t* context, const char* fmt, ...)
{
    CLOG_RET_VOID_IF_NULL(context);
    CLOG_RET_VOID_IF_NULL(fmt);
    const size_t len = strlen(context->err);
    CLOG_RET_VOID_IF(len >= sizeof(context->err) - 1);
    va_list args;
    va_start(args, fmt);
    const int ret = vsnprintf(context->err + len, sizeof(context->err) - len, fmt, args);
    va_end(args);
    if (ret <= 0) {
        context->err[0] = '\0';
    } else {
        CLOG_RET_VOID_IF(len + ret >= sizeof(context->err));
        context->err[len + ret] = '\n';
        context->err[len + ret + 1] = '\0';
    }
}

const char* clog_err_get(const clog_context_t* context)
{
    CLOG_RET_IF_NULL(context, "NULL");
    return context->err;
}
