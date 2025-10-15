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
#include <sys/time.h>
#include <time.h>
#endif
#ifdef CLOG_PLATFORM_LINUX
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(CLOG_PLATFORM_MACOS) || defined(CLOG_PLATFORM_UNIX)
#include <pthread.h>
#endif

typedef struct clog_context {
    const char* process;
    clog_hashmap_t* modules;
    clog_config_t config;
    struct {
        clog_placeholder_t placeholder;
        struct {
            struct {
                const char* trace;
                const char* debug;
                const char* info;
                const char* warn;
                const char* error;
                const char* fetal;
            } tag;
        } level;
    } formatter;
    struct {
        clog_filter_t pre;
        clog_filter_t post;
    } filters;
} clog_context_t;

static clog_context_t g_context = {0};

static char g_err[CASCA_LOG_ERR_BUF_SIZE] = {0}; /* NOT thread-safe */

static const clog_setup_f g_setup_funcs[] = {
    clog_formatter_setup,
    clog_filter_setup,
};

static const clog_cleanup_f g_cleanup_funcs[] = {
    clog_formatter_cleanup,
    clog_filter_cleanup,
};

static clog_res_e clog_init_process_attrs(const char* process, const clog_config_group_t* group)
{
    uint32_t value = 0;
    const clog_res_e ret = clog_config_find_item_in_group_uint(group, "level", &value);
    CLOG_RET_IF_X(ret != CLOG_SUCCESS, ret, "level not found or invalid in process[%s], ret = %d", process, ret);
    if (value > (CLOG_LEVEL_FETAL | CLOG_LEVEL_ERROR | CLOG_LEVEL_WARN | CLOG_LEVEL_INFO | CLOG_LEVEL_DEBUG |
                 CLOG_LEVEL_TRACE)) {
        clog_err_set("the value of level is invalid, value = %u", value);
        return CLOG_ERROR_FORMAT;
    }
    g_context.config.level = value;
    return CLOG_SUCCESS;
}

static clog_res_e clog_init_process_module(const char* process, const clog_config_group_t* module, clog_hashmap_t* map)
{
    bool enabled = true;
    CLOG_IGNORE_RES(clog_config_find_item_in_group_bool(module, "enabled", &enabled));
    CLOG_RET_IF(!enabled, CLOG_SUCCESS); /* module is not enabled, skip parse */
    uint32_t value = 0;
    const clog_res_e ret = clog_config_find_item_in_group_uint(module, "level", &value);
    if (ret == CLOG_TARGET_NOT_FOUND) {
        value = g_context.config.level;
    } else {
        CLOG_RET_IF_X(ret != CLOG_SUCCESS, ret, "level invalid in[%s.%s], ret = %d", process, module->name, ret);
    }
    if (value > (CLOG_LEVEL_FETAL | CLOG_LEVEL_ERROR | CLOG_LEVEL_WARN | CLOG_LEVEL_INFO | CLOG_LEVEL_DEBUG |
                 CLOG_LEVEL_TRACE)) {
        clog_err_set("the value of level is invalid, value = %u", value);
        return CLOG_ERROR_FORMAT;
    }
    const clog_module_t tmp = {
        .level = value,
    };
    return clog_hashmap_put(map, module->name, &tmp);
}

static clog_res_e clog_init_process(const char* process, const clog_config_group_t* root, clog_hashmap_t** modules)
{
    clog_hashmap_t* map =
        clog_hashmap_create(0, sizeof(clog_module_t), clog_hashmap_string_dup, clog_hashmap_string_free, NULL, NULL,
                            clog_hashmap_string_cmp, clog_hashmap_string_size, 0);
    CLOG_RET_IF_NULL_X(map, CLOG_NO_MEMORY, "clog_hashmap_create failed");
    const char* groups[] = {"Process", process};
    const clog_config_group_t* group = clog_config_find_group(root, groups, CLOG_ARRAY_SIZE(groups));
    if (group == NULL) {
        clog_hashmap_destroy(&map);
        clog_err_set("process [%s] not found", process);
        return CLOG_TARGET_NOT_FOUND;
    }
    clog_res_e ret = clog_init_process_attrs(process, group);
    if (ret != CLOG_SUCCESS) {
        clog_hashmap_destroy(&map);
        return ret;
    }
    const clog_config_group_t* child = group->child;
    while (child != NULL) {
        ret = clog_init_process_module(process, child, map);
        if (ret != CLOG_SUCCESS) {
            clog_hashmap_destroy(&map);
            return ret;
        }
        child = child->sibling;
    }
    *modules = map;
    return CLOG_SUCCESS;
}

clog_res_e clog_init(const char* process, const char* config)
{
    CLOG_RET_IF_NULL_X(process, CLOG_INVALID_PARAM, "process is NULL");
    CLOG_RET_IF_NULL_X(config, CLOG_INVALID_PARAM, "config is NULL");
    g_context.process = clog_strdup(process);
    CLOG_RET_IF_NULL_X(g_context.process, CLOG_NO_MEMORY, "clog_strdup process failed");

    g_context.config.root = clog_config_parse(config);
    clog_res_e ret = CLOG_ERROR_FORMAT;
    if (g_context.config.root == NULL) {
        clog_destroy(NULL, 0);
        return ret;
    }

    ret = clog_init_process(process, g_context.config.root, &g_context.modules);
    if (g_context.modules == NULL) {
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

const char* clog_get_level_tag(const clog_level_e level)
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

const clog_filter_t* clog_get_filters(const clog_filter_type_e type)
{
    if (type == CLOG_FILTER_PRE) {
        return g_context.filters.pre.next;
    }
    if (type == CLOG_FILTER_POST) {
        return g_context.filters.post.next;
    }
    return NULL;
}

void clog_set_filters(const clog_filter_t* pre, const clog_filter_t* post)
{
    g_context.filters.pre.next = pre;
    g_context.filters.post.next = post;
}

const clog_module_t* clog_get_module_info(const char* module)
{
    return clog_hashmap_get(g_context.modules, module);
}

void clog_destroy(const clog_cleanup_f* funcs, const size_t num)
{
    const size_t count = CLOG_ARRAY_SIZE(g_cleanup_funcs);
    for (size_t i = 0; i < count; i++) {
        g_cleanup_funcs[i]();
    }
    CLOG_SAFE_FREE(g_context.process);
    clog_filter_free((clog_filter_t*)g_context.filters.pre.next);
    g_context.filters.pre.next = NULL;
    clog_filter_free((clog_filter_t*)g_context.filters.post.next);
    g_context.filters.post.next = NULL;
    clog_config_destroy_group(g_context.config.root);
    clog_hashmap_destroy(&g_context.modules);
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
#elif defined(CLOG_PLATFORM_LINUX)
    return syscall(SYS_gettid);
#elif defined(CLOG_PLATFORM_MACOS)
    uint64_t tid;
    (void)pthread_threadid_np(NULL, &tid);
    return tid;
#elif defined(CLOG_PLATFORM_UNIX)
    return pthread_self() & 0xFFFFFFFF; /* ignore highest 4 bytes on 64Bit system */
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
    struct timeval tv;
    CLOG_IGNORE_RES(gettimeofday(&tv, NULL));
    struct tm tm_info;
    CLOG_IGNORE_RES(localtime_r(&tv.tv_sec, &tm_info));
    item->year = tm_info.tm_year + 1900;
    item->month = tm_info.tm_mon + 1;
    item->day = tm_info.tm_mday;
    item->hour = tm_info.tm_hour;
    item->minute = tm_info.tm_min;
    item->second = tm_info.tm_sec;
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

    clog_item_t item = {.filename = file,
                        .function = function,
                        .module = module,
                        .tid = clog_get_thread_id(),
                        .line = line,
                        .level = level,
                        .fmt = fmt};
    clog_item_init_datetime(&item);
    /* prefilter */
    bool pass = clog_filter_log(clog_get_filters(CLOG_FILTER_PRE), &item);
    CLOG_RET_IF(!pass, CLOG_NOT_PERMITTED);
    char* buf = clog_malloc(CASCA_LOG_SINGLE_LOG_MAX_SIZE);
    CLOG_RET_IF_NULL_X(buf, CLOG_NO_MEMORY, "malloc log content buf failed, size = %d", CASCA_LOG_SINGLE_LOG_MAX_SIZE);
    /* formatter */
    va_start(item.args, fmt);
    clog_res_e ret = clog_format_log(g_context.formatter.placeholder.next, &item, buf, CASCA_LOG_SINGLE_LOG_MAX_SIZE);
    va_end(item.args);
    if (ret != CLOG_SUCCESS) {
        clog_free(buf);
        return ret;
    }
    item.content = buf;
    /* postfilter */
    pass = clog_filter_log(clog_get_filters(CLOG_FILTER_POST), &item);
    if (!pass) {
        clog_free(buf);
        return CLOG_NOT_PERMITTED;
    }

    (void)printf("[===>]: %s\n", buf);

    /* TODO: pre-filter -> formatter -> post-filter -> dispatcher -> recorder */
    return CLOG_SUCCESS;
}
