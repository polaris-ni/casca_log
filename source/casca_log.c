/**
 * @author Polaris
 * @date  2025/9/3
 */

#include "casca_log.h"
#include <stdio.h>
#include "clog_config.h"
#include "clog_formatter.h"
#include "clog_hashmap.h"
#include "clog_mem_pool.h"
#include "clog_platform.h"
#include "clog_secure_func.h"
#ifdef CLOG_PLATFORM_WINDOWS
#include <windows.h>
#elif defined(CLOG_PLATFORM_MACOS) || defined(CLOG_PLATFORM_LINUX) || defined(CLOG_PLATFORM_UNIX)
#include <pthread.h>
#endif

static clog_context_t g_context = {0};

static char g_err[CASCA_LOG_ERR_BUF_SIZE] = {0}; /* NOT thread-safe */

static const clog_setup_f g_setup_funcs[] = {
    clog_formatter_setup,
};

static const clog_cleanup_f g_cleanup_funcs[] = {
    clog_formatter_cleanup,
};

clog_res_e clog_init(const char* process, const char* config)
{
    CLOG_RET_IF_NULL_X(process, CLOG_INVALID_PARAM, "process is NULL");
    CLOG_RET_IF_NULL_X(config, CLOG_INVALID_PARAM, "config is NULL");
    g_context.process = clog_strdup(process);
    if (g_context.process == NULL) {
        return CLOG_NO_MEMORY;
    }

    g_context.config.root = clog_config_parse(config);
    const clog_res_e ret = CLOG_ERROR_FORMAT;
    if (g_context.config.root == NULL) {
        clog_destroy(NULL, 0);
        return ret;
    }

#ifdef CASCA_LOG_MEM_POOL
    clog_mp_init(CLOG_MP_PRE_ALLOCATED_NORMAL);
#endif
    return CLOG_SUCCESS;
}

clog_res_e clog_setup(const clog_setup_f* funcs, const size_t num)
{
    CLOG_RET_IF_X(funcs == NULL && num != 0, CLOG_INVALID_PARAM, "funcs is NULL, but num is %zu", num);
    const size_t count = CLOG_ARRAY_SIZE(g_setup_funcs);
    clog_res_e ret;
    for (size_t i = 0; i < count; i++) {
        ret = g_setup_funcs[i]();
        CLOG_RET_IF(ret != CLOG_SUCCESS, ret);
    }
    CLOG_RET_IF(num == 0, CLOG_SUCCESS);
    for (size_t i = 0; i < num; ++i) {
        ret = funcs[i]();
        CLOG_RET_IF(ret != CLOG_SUCCESS, ret);
    }
    return CLOG_SUCCESS;
}

const clog_config_group_t* clog_get_config_root(void)
{
    return g_context.config.root;
}

const char* clog_get_process(void)
{
    return g_context.process == NULL ? "NULL" : g_context.process;
}

const char* clog_get_level_tag(clog_level_e level)
{
    switch (level) {
        case CLOG_LEVEL_TRACE:
            return g_context.formatter.level.tag.trace;
        case CLOG_LEVEL_DEBUG:
            return g_context.formatter.level.tag.debug;
        case CLOG_LEVEL_INFO:
            return g_context.formatter.level.tag.info;
        case CLOG_LEVEL_WARN:
            return g_context.formatter.level.tag.warn;
        case CLOG_LEVEL_ERROR:
            return g_context.formatter.level.tag.error;
        case CLOG_LEVEL_FETAL:
            return g_context.formatter.level.tag.fetal;
        default:
            return "NULL";
    }
}

void clog_set_level_tag(const clog_level_e level, const char* tag)
{
    switch (level) {
        case CLOG_LEVEL_TRACE:
            g_context.formatter.level.tag.trace = tag;
            break;
        case CLOG_LEVEL_DEBUG:
            g_context.formatter.level.tag.debug = tag;
            break;
        case CLOG_LEVEL_INFO:
            g_context.formatter.level.tag.info = tag;
            break;
        case CLOG_LEVEL_WARN:
            g_context.formatter.level.tag.warn = tag;
            break;
        case CLOG_LEVEL_ERROR:
            g_context.formatter.level.tag.error = tag;
            break;
        case CLOG_LEVEL_FETAL:
            g_context.formatter.level.tag.fetal = tag;
            break;
        default:
            clog_err_set("level %u is invalid", level);
            break;
    }
}

const clog_placeholder_t* clog_get_placeholders(void)
{
    return g_context.formatter.placeholder.next;
}

void clog_set_placeholders(const clog_placeholder_t* placeholder)
{
    g_context.formatter.placeholder.next = placeholder;
}

void clog_destroy(const clog_cleanup_f* funcs, const size_t num)
{
    const size_t count = CLOG_ARRAY_SIZE(g_cleanup_funcs);
    for (size_t i = 0; i < count; i++) {
        g_cleanup_funcs[i]();
    }
    CLOG_SAFE_FREE(g_context.process);
    clog_config_destroy_group(g_context.config.root);
    clog_mp_finalize();
    g_context.config.root = NULL;
    g_context.config.level = CLOG_LEVEL_OFF;
    if (funcs != NULL && num != 0) {
        for (size_t i = 0; i < num; ++i) {
            funcs[i]();
        }
    }
}

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

static unsigned long long clog_get_thread_id(void)
{
#ifdef CLOG_PLATFORM_WINDOWS
    return GetCurrentThreadId();
#elif defined(CLOG_PLATFORM_MACOS) || defined(CLOG_PLATFORM_LINUX) || defined(CLOG_PLATFORM_UNIX)
    return (unsigned long long)pthread_self();
#else
    return 0;
#endif
}

static void clog_item_init_datetime(clog_item_t* item)
{
#ifdef CLOG_PLATFORM_WINDOWS
    SYSTEMTIME st;
    GetLocalTime(&st);
    item->year = st.wYear;
    item->month = st.wMonth;
    item->day = st.wDay;
    item->hour = st.wHour;
    item->minute = st.wMinute;
    item->second = st.wSecond;
    item->millisecond = st.wMilliseconds;
#elif defined(CLOG_PLATFORM_MACOS) || defined(CLOG_PLATFORM_LINUX) || defined(CLOG_PLATFORM_UNIX)
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    struct tm* tm_info = localtime(&tv.tv_sec);
    item->year = tm_info->tm_year + 1900;
    item->month = tm_info->tm_mon + 1;
    item->day = tm_info->tm_mday;
    item->hour = tm_info->tm_hour;
    item->minute = tm_info->tm_min;
    item->second = tm_info->tm_sec;
    item->millisecond = tv.tv_usec / 1000;
#else
    item->year = 1970;
    item->month = 1;
    item->day = 1;
    item->hour = 0;
    item->minute = 0;
    item->second = 0;
    item->millisecond = 0;
#endif
}

clog_res_e clog_log(const uint32_t* recorders, const size_t count, const char* module, const char* file,
                    const char* function, const int line, const clog_level_e level, const char* fmt, ...)
{
    char* content = clog_malloc(CASCA_LOG_SINGLE_LOG_MAX_SIZE);
    CLOG_RET_IF_NULL_X(content, CLOG_NO_MEMORY, "malloc log content buf failed, size = %d",
                       CASCA_LOG_SINGLE_LOG_MAX_SIZE);
    clog_item_t item = {.filepath = file,
                        .filename = file,
                        .function = function,
                        .module = module,
                        .tid = clog_get_thread_id(),
                        .line = line,
                        .level = level,
                        .content = content};
    va_list args;
    va_start(args, fmt);
    CLOG_IGNORE_RES(vsnprintf((char*)item.content, CASCA_LOG_SINGLE_LOG_MAX_SIZE, fmt, args));
    va_end(args);
    clog_item_init_datetime(&item);
    const size_t num = count > CASCA_LOG_TARGET_RECORDER_COUNT ? CASCA_LOG_TARGET_RECORDER_COUNT : count;
    uint32_t* ptr = (uint32_t*)item.recorders;
    for (int i = 0; i < CASCA_LOG_TARGET_RECORDER_COUNT; ++i) {
        if (i < num) {
            ptr[i] = recorders[i];
        } else {
            ptr[i] = CLOG_RECORDER_ID_INVALID;
        }
    }
    /* TODO: pre-filter -> formatter -> post-filter -> dispatcher -> recorder */
    return CLOG_SUCCESS;
}
