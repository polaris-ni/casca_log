/**
 * @auther polaris
 * @date  2025/10/18
 */
#include "clog_error.h"
#include <stdio.h>
#include <string.h>

#include "clog_config.h"

static char g_err[CASCA_LOG_ERR_BUF_SIZE] = {0}; /* NOT thread-safe */

void clog_err_clear(void)
{
    g_err[0] = '\0';
}

void clog_err_set(const char* fmt, ...)
{
    CLOG_RET_VOID_IF_NULL(fmt);
    va_list args;
    va_start(args, fmt);
    const int ret = vsnprintf(g_err, sizeof(g_err), fmt, args);
    va_end(args);
    if (ret <= 0) {
        clog_err_clear();
    }
}

void clog_err_append(const char* fmt, ...)
{
    CLOG_RET_VOID_IF_NULL(fmt);
    const size_t len = strlen(g_err);
    CLOG_RET_VOID_IF(len >= sizeof(g_err) - 1);
    va_list args;
    va_start(args, fmt);
    const int ret = vsnprintf(g_err + len, sizeof(g_err) - len, fmt, args);
    va_end(args);
    if (ret <= 0) {
        g_err[len - 1] = '\0';
    }
}

void clog_err_append_line(const char* fmt, ...)
{
    CLOG_RET_VOID_IF_NULL(fmt);
    const size_t len = strlen(g_err);
    CLOG_RET_VOID_IF(len >= sizeof(g_err) - 1);
    va_list args;
    va_start(args, fmt);
    const int ret = vsnprintf(g_err + len, sizeof(g_err) - len, fmt, args);
    va_end(args);
    if (ret <= 0) {
        g_err[len - 1] = '\0';
    } else {
        CLOG_RET_VOID_IF(len + ret >= sizeof(g_err));
        g_err[len + ret] = '\n';
        g_err[len + ret + 1] = '\0';
    }
}

const char* clog_err_get(void)
{
    return g_err;
}
