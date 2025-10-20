/**
 * @auther polaris
 * @date  2025/10/18
 */
#include "clog_error.h"
#include <stdio.h>
#include "clog_atomic.h"
#include "clog_config.h"
#include "clog_hooks.h"

static char** g_err_msg = NULL;
static uint16_t g_err_num = 0;
static uint16_t g_err_line_size = 0;
static atomic_uint g_err_count;

void clog_err_setup(unsigned short num, unsigned short size)
{
    CLOG_RET_VOID_IF((num == 0) || (size == 0));
    g_err_msg = clog_malloc(num * sizeof(char*));
    CLOG_RET_VOID_IF_NULL(g_err_msg);
    for (unsigned short i = 0; i < num; ++i) {
        g_err_msg[i] = clog_malloc(size);
        if (g_err_msg[i] == NULL) {
            for (unsigned short j = 0; j < i; ++j) {
                CLOG_SAFE_FREE(g_err_msg[j]);
            }
            CLOG_SAFE_FREE(g_err_msg);
            return;
        }
    }
    g_err_num = num;
    g_err_line_size = size;
    atomic_init(&g_err_count, 0);
}

static uint32_t clog_err_get_index(void)
{
    uint32_t new_index = 0;
    uint32_t old_index = 0;
    do {
        old_index = (uint32_t)atomic_load(&g_err_count) & UINT32_MAX;
        CLOG_RET_IF(old_index >= g_err_num, g_err_num);
        new_index = old_index + 1;
    } while (!atomic_compare_exchange_strong(&g_err_count, &old_index, new_index));
    return old_index;
}

void clog_err_put(const char* file, int line, const char* fmt, ...)
{
    CLOG_RET_VOID_IF(g_err_num == 0);
    const uint32_t index = clog_err_get_index();
    CLOG_RET_VOID_IF(index >= g_err_num);
    char* log = g_err_msg[index];
    int ret = snprintf(log, g_err_line_size, "[%u %s:%d] ", index, file, line);
    if (ret < 0) {
        ret = 0;
    }
    CLOG_RET_VOID_IF(ret > g_err_line_size);
    va_list args;
    va_start(args, fmt);
    CLOG_IGNORE_RES(vsnprintf(log + ret, g_err_line_size - ret, fmt, args));
    va_end(args);
}

const char** clog_err_get(unsigned int* num)
{
    CLOG_RET_IF_NULL(num, NULL);
    *num = 0;
    CLOG_RET_IF(g_err_num == 0, NULL);
    *num = atomic_load(&g_err_count);
    CLOG_RET_IF(*num == 0, NULL);
    return (const char**)g_err_msg;
}

void clog_err_print(char* buf, unsigned int size, const char* separator)
{
    CLOG_RET_VOID_IF_NULL(buf);
    CLOG_RET_VOID_IF(size == 0);
    uint32_t num = 0;
    const char** errors = clog_err_get(&num);
    CLOG_RET_VOID_IF_NULL(errors);
    CLOG_RET_VOID_IF(num == 0);
    size_t offset = 0;
    if (separator == NULL) {
        separator = "\n";
    }
    int ret = 0;
    for (uint32_t i = 0; i < num; ++i) {
        if (i != num - 1) {
            ret = snprintf(buf + offset, size - offset, "%s%s", g_err_msg[i], separator);
        } else {
            ret = snprintf(buf + offset, size - offset, "%s", g_err_msg[i]);
        }
        if (ret < 0) {
            continue;
        }
        offset += ret;
        if (offset >= size) {
            return;
        }
    }
}

void clog_err_clear(void)
{
    atomic_store(&g_err_count, 0);
}

void clog_err_cleanup(void)
{
    for (size_t i = 0; i < g_err_num; ++i) {
        CLOG_SAFE_FREE(g_err_msg[i]);
    }
    CLOG_SAFE_FREE(g_err_msg);
    g_err_num = 0;
    g_err_line_size = 0;
    atomic_init(&g_err_count, 0);
}
