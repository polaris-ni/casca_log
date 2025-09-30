/**
 * @auther Polaris
 * @date  2025/9/19
 */
#include "clog_placeholder.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "casca_log.h"
#include "clog_hooks.h"
#include "clog_secure_func.h"

static const char* g_tags[] = {NULL, NULL, NULL, NULL, NULL, NULL};

/* << default placeholder implementation start */

static size_t clog_num_to_str(uint32_t value, const uint32_t num, char* buf, const size_t size, const bool is_padding)
{
    (void)size;
    static const char digits[] = "0123456789";
    if (value == 0) {
        buf[0] = '0';
        return 1;
    }
    if (is_padding) {
        unsigned int tmp = num;
        while (tmp-- > 0) {
            buf[tmp] = digits[value % 10];
            value /= 10;
        }
        return num;
    }
    static const unsigned int max_values[] = {0, 10, 100, 1000, 10000, 100000};
    uint32_t index = num;
    while (max_values[index] > value) {
        index--;
    }
    uint32_t i = 0;
    while (index != 0) {
        buf[i] = digits[value / max_values[index]];
        value = value % max_values[index];
        i++;
        index--;
    }
    return i;
}

#define CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(name, max, num, is_padding)                        \
    static inline size_t clog_placeholder_##name(const clog_item_t* item, char* buf, const size_t size) \
    {                                                                                                   \
        CLOG_RET_IF(size < num, 0);                                                                     \
        CLOG_ASSERT(item != NULL);                                                                      \
        CLOG_ASSERT(buf != NULL);                                                                       \
        CLOG_ASSERT(item->name <= max);                                                                 \
        return clog_num_to_str(item->name, num, buf, size, is_padding);                                 \
    }

CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(year, 9999, 4, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(month, 12, 2, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(day, 31, 2, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(hour, 23, 2, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(minute, 59, 2, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(second, 59, 2, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(millisecond, 999, 3, true)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(line, 999999, 6, false)
CLOG_DECLARE_PLACEHOLDER_NUM_FORMAT_FUNCTION(tid, 9999999999, 10, false)

static inline size_t clog_placeholder_string_copy(const char* str, char* buf, const size_t size)
{
    const char* p = str;
    size_t i = 0;
    while (*p != '\0') {
        if (i == size) {
            return i;
        }
        buf[i++] = *p++;
    }
    return i;
}

static size_t clog_placeholder_process(const clog_item_t* item, char* buf, const size_t size)
{
    CLOG_UNUSED_VAR(item);
    return clog_placeholder_string_copy(clog_get_process(), buf, size);
}

static size_t clog_placeholder_module(const clog_item_t* item, char* buf, const size_t size)
{
    return clog_placeholder_string_copy(item->module, buf, size);
}

static size_t clog_placeholder_file(const clog_item_t* item, char* buf, const size_t size)
{
    return clog_placeholder_string_copy(item->filename, buf, size);
}

static size_t clog_placeholder_content(const clog_item_t* item, char* buf, const size_t size)
{
    return clog_placeholder_string_copy(item->content, buf, size);
}

static size_t clog_placeholder_ln(const clog_item_t* item, char* buf, const size_t size)
{
    CLOG_UNUSED_VAR(item);
    CLOG_UNUSED_VAR(size);
    buf[0] = '\n';
    return 1;
}

static size_t clog_placeholder_level(const clog_item_t* item, char* buf, const size_t size)
{
    switch (item->level) {
        case CLOG_LEVEL_TRACE:
            CLOG_RET_IF_NULL(g_tags[0], 0);
            return clog_placeholder_string_copy(g_tags[0], buf, size);
        case CLOG_LEVEL_DEBUG:
            CLOG_RET_IF_NULL(g_tags[1], 0);
            return clog_placeholder_string_copy(g_tags[1], buf, size);
        case CLOG_LEVEL_INFO:
            CLOG_RET_IF_NULL(g_tags[2], 0);
            return clog_placeholder_string_copy(g_tags[2], buf, size);
        case CLOG_LEVEL_WARN:
            CLOG_RET_IF_NULL(g_tags[3], 0);
            return clog_placeholder_string_copy(g_tags[3], buf, size);
        case CLOG_LEVEL_ERROR:
            CLOG_RET_IF_NULL(g_tags[4], 0);
            return clog_placeholder_string_copy(g_tags[4], buf, size);
        case CLOG_LEVEL_FETAL:
            CLOG_RET_IF_NULL(g_tags[5], 0);
            return clog_placeholder_string_copy(g_tags[5], buf, size);
        default:
            return 0;
    }
}


#define CLOG_PLACEHOLDER_DECLARE(n, f, i) {.name = n, .func = f, .next = &g_placeholder_list[i + 1]}

#define CLOG_PLACEHOLDER_DECLARE_LAST(n, f) {.name = n, .func = f, .next = NULL}

static clog_placeholder_t g_placeholder_list[] = {
    CLOG_PLACEHOLDER_DECLARE("_year", clog_placeholder_year, 0),
    CLOG_PLACEHOLDER_DECLARE("_month", clog_placeholder_month, 1),
    CLOG_PLACEHOLDER_DECLARE("_day", clog_placeholder_day, 2),
    CLOG_PLACEHOLDER_DECLARE("_hour", clog_placeholder_hour, 3),
    CLOG_PLACEHOLDER_DECLARE("_minute", clog_placeholder_minute, 4),
    CLOG_PLACEHOLDER_DECLARE("_second", clog_placeholder_second, 5),
    CLOG_PLACEHOLDER_DECLARE("_millisecond", clog_placeholder_millisecond, 6),
    CLOG_PLACEHOLDER_DECLARE("_process", clog_placeholder_process, 7),
    CLOG_PLACEHOLDER_DECLARE("_module", clog_placeholder_module, 8),
    CLOG_PLACEHOLDER_DECLARE("_tid", clog_placeholder_tid, 9),
    CLOG_PLACEHOLDER_DECLARE("_file", clog_placeholder_file, 10),
    CLOG_PLACEHOLDER_DECLARE("_line", clog_placeholder_line, 11),
    CLOG_PLACEHOLDER_DECLARE("_content", clog_placeholder_content, 12),
    CLOG_PLACEHOLDER_DECLARE("_level", clog_placeholder_level, 13),
    CLOG_PLACEHOLDER_DECLARE_LAST("_ln", clog_placeholder_ln),
};
#undef CLOG_PLACEHOLDER_DECLARE_LAST
#undef CLOG_PLACEHOLDER_DECLARE

static bool clog_placeholder_is_name_valid(const char ch)
{
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_') {
        return true;
    }
    return false;
}

clog_res_e clog_placeholder_register(const char* name, const clog_placeholder_f func)
{
    CLOG_RET_IF_NULL(name, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(func, CLOG_INVALID_PARAM);
    /* customize placeholder should not start with '_' */
    if (!clog_placeholder_is_name_valid(name[0]) || name[0] == '_') {
        clog_err_set("placeholder name [%s] not start with [a-z, A-Z, 0-9]", name);
        return CLOG_INVALID_PARAM;
    }

    clog_placeholder_t* last = &g_placeholder_list[CLOG_ARRAY_SIZE(g_placeholder_list) - 1];
    clog_placeholder_t* placeholder = clog_malloc(sizeof(clog_placeholder_t));
    CLOG_RET_IF_NULL(placeholder, CLOG_NO_MEMORY);
    placeholder->name = clog_strdup(name);
    if (placeholder->name == NULL) {
        clog_free(placeholder);
        return CLOG_NO_MEMORY;
    }
    placeholder->func = func;
    placeholder->next = last->next;
    last->next = placeholder;
    return CLOG_SUCCESS;
}

static clog_placeholder_t* clog_placeholder_create(const char* name, const bool is_placeholder)
{
    clog_placeholder_t* placeholder = clog_malloc(sizeof(clog_placeholder_t));
    CLOG_RET_IF_NULL(placeholder, NULL);
    placeholder->name = name;
    placeholder->next = NULL;
    if (!is_placeholder) {
        placeholder->func = NULL;
        return placeholder;
    }

    const clog_placeholder_t* tmp = &g_placeholder_list[0];
    while (tmp != NULL) {
        if (strcmp(tmp->name, name) == 0) {
            placeholder->func = tmp->func;
            return placeholder;
        }
        tmp = tmp->next;
    }
    clog_err_append_line("process function of placeholder {%s} not found", name);
    clog_free(placeholder);
    return NULL;
}

/* {hello} -> hello */
static const char* clog_placeholder_parse_name(const char* format)
{
    const char* tmp = format + 1;
    if (*tmp == '}') {
        clog_err_append_line("placeholder name empty {}");
        return NULL;
    }

    while (*tmp != '\0') {
        if (*tmp == '}') {
            return tmp - 1;
        }
        if (!clog_placeholder_is_name_valid(*tmp)) {
            clog_err_append_line("placeholder contains invalid char [%c]", *tmp);
            return NULL;
        }
        tmp++;
    }
    clog_err_append_line("placeholder does not contain '}'");
    return NULL;
}

static clog_res_e clog_placeholder_parse_format(const char* format, clog_placeholder_t* root)
{
    CLOG_RET_IF(format[0] == '\0', CLOG_SUCCESS); /* parse over */
    const char* tmp = format;
    if (*tmp == '{') {
        tmp = clog_placeholder_parse_name(format);
        CLOG_RET_IF_NULL(tmp, CLOG_INVALID_PARAM);
        char* name = clog_strndup(format + 1, tmp - format);
        CLOG_RET_IF_NULL(name, CLOG_NO_MEMORY);
        clog_placeholder_t* placeholder = clog_placeholder_create(name, true);
        if (placeholder == NULL) {
            clog_free(name);
            return CLOG_FAIL;
        }
        root->next = placeholder;
        return clog_placeholder_parse_format(tmp + 2, placeholder);
    }
    bool is_escape = false;
    while (*tmp != '\0') {
        if (*tmp == '{') {
            if (!is_escape) {
                break;
            }
        }
        if (*tmp == '}') {
            if (!is_escape) {
                clog_err_append_line("there is no corresponding '{' for '}'");
                return false;
            }
        }
        is_escape = *tmp == '\\';
        tmp++;
    }
    char* name = clog_malloc(tmp - format + 1);
    CLOG_RET_IF_NULL(name, CLOG_NO_MEMORY);
    const char* start = format;
    while (start < tmp) {
        if (*start == '\\') {
            if (*(start + 1) == '{' || *(start + 1) == '}') {
                start++;
                continue;
            }
        }
        *name = *start;
        name++;
        start++;
    }
    *name = '\0';
    clog_placeholder_t* placeholder = clog_placeholder_create(name, false);
    if (placeholder == NULL) {
        clog_free(name);
        return CLOG_FAIL;
    }
    root->next = placeholder;
    return clog_placeholder_parse_format(tmp, placeholder);
}

clog_res_e clog_placeholder_parse(const char* format, clog_placeholder_t* root)
{
    CLOG_RET_IF_NULL(format, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(root, CLOG_INVALID_PARAM);
    CLOG_RET_IF_X(root->next != NULL, CLOG_INVALID_PARAM, "root.next is not null, please clear first");
    clog_err_clear();
    const clog_res_e ret = clog_placeholder_parse_format(format, root);
    if (ret != CLOG_SUCCESS) {
        clog_placeholder_clear(root);
        clog_err_append_line("parse format [%s] failed", format);
    }
    /* preload tag str */
    g_tags[0] = clog_get_level_tag(CLOG_LEVEL_TRACE);
    g_tags[1] = clog_get_level_tag(CLOG_LEVEL_DEBUG);
    g_tags[2] = clog_get_level_tag(CLOG_LEVEL_INFO);
    g_tags[3] = clog_get_level_tag(CLOG_LEVEL_WARN);
    g_tags[4] = clog_get_level_tag(CLOG_LEVEL_ERROR);
    g_tags[5] = clog_get_level_tag(CLOG_LEVEL_FETAL);
    return ret;
}

void clog_placeholder_clear(clog_placeholder_t* root)
{
    CLOG_RET_VOID_IF(root);
    CLOG_SAFE_FREE(root->name);
    root->func = NULL;
    const clog_placeholder_t* cur = root->next;
    while (cur != NULL) {
        const clog_placeholder_t* next = cur->next;
        clog_free((void*)cur->name);
        clog_free((void *)cur);
        cur = next;
    }
}
