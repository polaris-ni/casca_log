/**
 * @auther Polaris
 * @date  2025/9/19
 */
#include "clog_log_format_placeholder.h"
#include <stdbool.h>
#include <stdio.h>
#include "casca_log.h"
#include "clog_error.h"
#include "clog_hooks.h"

static clog_hashmap_t *g_placeholder_map = NULL;

/* << default placeholder implementation start */

static size_t clog_num_to_str(uint32_t value, size_t num, char *buf, const size_t size, const bool is_padding)
{
    CLOG_UNUSED_VAR(size);
    static const char digits[] = "0123456789";
    if (is_padding) {
        size_t tmp = num;
        while (tmp-- > 0) {
            buf[tmp] = digits[value % 10];
            value /= 10;
        }
        return num;
    }
    if (value == 0) {
        buf[0] = '0';
        return 1;
    }
    static const unsigned int max_values[] = {1,       10,       100,       1000,       10000,     100000,
                                              1000000, 10000000, 100000000, 1000000000, 1000000000};
    int32_t index = (int32_t)num;
    while (max_values[index] > value) {
        index--;
    }
    size_t i = 0;
    while (index >= 0) {
        buf[i] = digits[value / max_values[index]];
        value = value % max_values[index];
        i++;
        index--;
    }
    return i;
}

#define CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(name, max, num, is_padding)    \
    static int clog_placeholder_##name(void *param, char *buf, size_t size)         \
    {                                                                               \
        CLOG_RET_IF(size < (num), -CLOG_INVALID_PARAM);                             \
        CLOG_ASSERT(param != NULL);                                                 \
        CLOG_ASSERT(buf != NULL);                                                   \
        clog_item_wrapper_t *wrapper = (clog_item_wrapper_t *)param;                \
        CLOG_ASSERT(wrapper->log->name <= (max));                                   \
        return clog_num_to_str(wrapper->log->name, (num), buf, size, (is_padding)); \
    }

CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(year, 9999, 4, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(month, 12, 2, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(day, 31, 2, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(hour, 23, 2, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(minute, 59, 2, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(second, 59, 2, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(millisecond, 999, 3, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(line, 999999, 6, false)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(tid, UINT32_MAX, 10, false)
#undef CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION

static int clog_placeholder_string_copy(const char *str, char *buf, const size_t size)
{
    const char *p = str;
    int i = 0;
    while (*p != '\0') {
        if (i == size) {
            return i;
        }
        buf[i++] = *p++;
    }
    return i;
}

static int clog_placeholder_process(void *param, char *buf, size_t size)
{
    CLOG_UNUSED_VAR(param);
    return clog_placeholder_string_copy(clog_get_process(), buf, size);
}

static int clog_placeholder_module(void *param, char *buf, size_t size)
{
    const clog_item_wrapper_t *wrapper = (clog_item_wrapper_t *)param;
    return clog_placeholder_string_copy(wrapper->module, buf, size);
}

static int clog_placeholder_file(void *param, char *buf, size_t size)
{
    const clog_item_wrapper_t *wrapper = (clog_item_wrapper_t *)param;
    return clog_placeholder_string_copy(wrapper->filename, buf, size);
}

static int clog_placeholder_content(void *param, char *buf, size_t size)
{
    const clog_item_wrapper_t *wrapper = (clog_item_wrapper_t *)param;
    const int ret = vsnprintf(buf, size, wrapper->fmt, ((clog_item_wrapper_t *)wrapper)->args);
    CLOG_RET_IF(ret < 0, 0);
    return ret;
}

static int clog_placeholder_ln(void *param, char *buf, size_t size)
{
    CLOG_UNUSED_VAR(param);
    CLOG_UNUSED_VAR(size);
    buf[0] = '\n';
    return 1;
}

static int clog_placeholder_level(void *param, char *buf, size_t size)
{
    const clog_item_wrapper_t *wrapper = (clog_item_wrapper_t *)param;
    return clog_placeholder_string_copy(clog_get_level_tag(wrapper->log->level), buf, size);
}

clog_res_e clog_log_format_placeholder_register(const char *name, clog_placeholder_handler_f handler)
{
    CLOG_RET_IF_NULL(name, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(handler, CLOG_INVALID_PARAM);
    /* customize placeholder should not start with '_' */
    CLOG_RET_IF_X(name[0] == '_', CLOG_INVALID_PARAM, "placeholder name [%s] not start with [a-z, A-Z, 0-9]", name);
    clog_hashmap_t *map = clog_log_format_placeholder_get_map();
    CLOG_RET_IF_NULL_X(map, CLOG_INVALID_PARAM, "log format placeholder map is NULL");
    return clog_hashmap_put(map, name, handler);
}

clog_hashmap_t *clog_log_format_placeholder_get_map(void)
{
    if (g_placeholder_map != NULL) {
        return g_placeholder_map;
    }
    g_placeholder_map =
        clog_hashmap_create(0, sizeof(clog_placeholder_handler_f), clog_hashmap_string_dup, clog_hashmap_string_free,
                            NULL, NULL, clog_hashmap_string_cmp, clog_hashmap_string_size, 0);
    CLOG_RET_IF_NULL_X(g_placeholder_map, NULL, "clog_hashmap_create failed");
    const char *default_placeholder_names[] = {"_year",   "_month",       "_day",     "_hour",   "_minute",
                                               "_second", "_millisecond", "_process", "_module", "_tid",
                                               "_file",   "_line",        "_content", "_level",  "_ln"};
    const clog_placeholder_handler_f default_placeholder_handlers[] = {
        clog_placeholder_year,    clog_placeholder_month,  clog_placeholder_day,         clog_placeholder_hour,
        clog_placeholder_minute,  clog_placeholder_second, clog_placeholder_millisecond, clog_placeholder_process,
        clog_placeholder_module,  clog_placeholder_tid,    clog_placeholder_file,        clog_placeholder_line,
        clog_placeholder_content, clog_placeholder_level,  clog_placeholder_ln};
    CLOG_ASSERT(CLOG_ARRAY_SIZE(default_placeholder_names) == CLOG_ARRAY_SIZE(default_placeholder_handlers));
    const size_t default_placeholder_num = CLOG_ARRAY_SIZE(default_placeholder_names);
    for (size_t i = 0; i < default_placeholder_num; i++) {
        const clog_res_e ret =
            clog_hashmap_put(g_placeholder_map, default_placeholder_names[i], &default_placeholder_handlers[i]);
        CLOG_CLEAN_RET_IF_X(ret != CLOG_SUCCESS, clog_hashmap_destroy(&g_placeholder_map), NULL,
                            "add default log format placeholder %s failed, ret = %u", default_placeholder_names[i],
                            ret);
    }
    return g_placeholder_map;
}

void clog_log_format_placeholder_clear(void)
{
    if (g_placeholder_map != NULL) {
        clog_hashmap_destroy(&g_placeholder_map);
    }
}
