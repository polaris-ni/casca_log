/**
 * @auther Polaris
 * @date  2025/9/19
 */
#include "clog_placeholder.h"

#include <stdio.h>
#include <time.h>
#include "casca_log.h"
#include "clog_config.h"
#include "clog_hooks.h"
#include "clog_secure_func.h"

#define CLOG_PLACEHOLDER_DECLARE(n, f, i) {.name = n, .func = f, .next = &g_placeholder_list[i + 1]}

#define CLOG_PLACEHOLDER_DECLARE_LAST(n, f) {.name = n, .func = f, .next = NULL}

static clog_placeholder_t g_placeholder_list[] = {
    CLOG_PLACEHOLDER_DECLARE("_year", clog_placeholder_year, 0),
    CLOG_PLACEHOLDER_DECLARE("_month", clog_placeholder_month, 1),
    CLOG_PLACEHOLDER_DECLARE("_day", clog_placeholder_day, 2),
    CLOG_PLACEHOLDER_DECLARE("_hour", clog_placeholder_hour, 3),
    CLOG_PLACEHOLDER_DECLARE("_minute", clog_placeholder_minute, 4),
    CLOG_PLACEHOLDER_DECLARE("_second", clog_placeholder_second, 5),
    CLOG_PLACEHOLDER_DECLARE_LAST("_millisecond", clog_placeholder_millisecond),
};

size_t clog_placeholder_year(const clog_context_t* ctx, const clog_item_t* item, char* buf, const size_t size)
{
    CLOG_RET_IF(size < 4, 0);
    const int ret = sprintf(buf, "%04u", item->year);
    CLOG_RET_IF(ret < 0, 0);
    return ret;
}

size_t clog_placeholder_month(const clog_context_t* ctx, const clog_item_t* item, char* buf, const size_t size)
{
    CLOG_RET_IF(size < 4, 0);
    const int ret = sprintf(buf, "%02u", item->month);
    CLOG_RET_IF(ret < 0, 0);
    return ret;
}

size_t clog_placeholder_day(const clog_context_t* ctx, const clog_item_t* item, char* buf, const size_t size)
{
    CLOG_RET_IF(size < 4, 0);
    const int ret = sprintf(buf, "%02u", item->day);
    CLOG_RET_IF(ret < 0, 0);
    return ret;
}

size_t clog_placeholder_hour(const clog_context_t* ctx, const clog_item_t* item, char* buf, const size_t size)
{
    CLOG_RET_IF(size < 4, 0);
    const int ret = sprintf(buf, "%02u", item->hour);
    CLOG_RET_IF(ret < 0, 0);
    return ret;
}

size_t clog_placeholder_minute(const clog_context_t* ctx, const clog_item_t* item, char* buf, const size_t size)
{
    CLOG_RET_IF(size < 4, 0);
    const int ret = sprintf(buf, "%02u", item->minute);
    CLOG_RET_IF(ret < 0, 0);
    return ret;
}

size_t clog_placeholder_second(const clog_context_t* ctx, const clog_item_t* item, char* buf, const size_t size)
{
    CLOG_RET_IF(size < 4, 0);
    const int ret = sprintf(buf, "%02u", item->second);
    CLOG_RET_IF(ret < 0, 0);
    return ret;
}

size_t clog_placeholder_millisecond(const clog_context_t* ctx, const clog_item_t* item, char* buf, size_t size)
{
    CLOG_RET_IF(size < 4, 0);
    const int ret = sprintf(buf, "%03u", item->millisecond);
    CLOG_RET_IF(ret < 0, 0);
    return ret;
}

static bool clog_placeholder_is_name_valid(const char ch)
{
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
        return false;
    }
    return true;
}

clog_res_e clog_placeholder_register(clog_context_t* context, const char* name, const clog_placeholder_f func)
{
    CLOG_RET_IF_NULL(context, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(name, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(func, CLOG_INVALID_PARAM);
    if (!clog_placeholder_is_name_valid(name[0])) {
        clog_err_set(context, "placeholder name [%s] not start with [a-z, A-Z, 0-9]", name);
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

static clog_placeholder_t* clog_placeholder_create(clog_context_t* ctx, const char* name, const bool is_placeholder)
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
    clog_err_append_line(ctx, "process function of placeholder {%s} not found", name);
    clog_free(placeholder);
    return NULL;
}

/* {hello} -> hello */
static const char* clog_placeholder_parse_name(clog_context_t* context, const char* format)
{
    const char* tmp = format + 1;
    if (*tmp == '}') {
        clog_err_append_line(context, "placeholder name empty {}");
        return NULL;
    }

    while (*tmp != '\0') {
        if (*tmp == '}') {
            return tmp - 1;
        }
        if (!clog_placeholder_is_name_valid(*tmp)) {
            clog_err_append_line(context, "placeholder contains invalid char [%c]", *tmp);
            return NULL;
        }
        tmp++;
    }
    clog_err_append_line(context, "placeholder does not contain '}'");
    return NULL;
}

static clog_res_e clog_placeholder_parse_format(clog_context_t* ctx, clog_placeholder_t* root, const char* format)
{
    CLOG_RET_IF(format[0] == '\0', CLOG_SUCCESS); /* parse over */
    const char* tmp = format;
    if (*tmp == '{') {
        tmp = clog_placeholder_parse_name(ctx, format);
        if (tmp == NULL) {
            return CLOG_ERROR_FORMAT;
        }
        char* name = clog_strndup(format, tmp - format + 1);
        CLOG_RET_IF_NULL(name, CLOG_NO_MEMORY);
        clog_placeholder_t* placeholder = clog_placeholder_create(ctx, name, true);
        if (placeholder == NULL) {
            clog_free(name);
            return CLOG_FAIL;
        }
        root->next = placeholder;
        return clog_placeholder_parse_format(ctx, placeholder, tmp + 2);
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
                clog_err_append_line(ctx, "there is no corresponding '{' for '}'");
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
    clog_placeholder_t* placeholder = clog_placeholder_create(ctx, name, false);
    if (placeholder == NULL) {
        clog_free(name);
        return CLOG_FAIL;
    }
    root->next = placeholder;
    return clog_placeholder_parse_format(ctx, placeholder, tmp);
}

clog_res_e clog_placeholder_parse(clog_context_t* context)
{
    CLOG_RET_IF_NULL(context, CLOG_INVALID_PARAM);
    clog_err_clear(context);
    const char* groups[] = {"Formatter"};
    const clog_config_group_t* root = context->config.raw;
    const clog_config_item_t* item = clog_config_find_item(root, groups, CLOG_ARRAY_SIZE(groups), "format");
    CLOG_RET_IF_NULL_X(context, item, CLOG_TARGET_NOT_FOUND, "item \"format\" of group [Formatter] not found");
    CLOG_RET_IF_X(context, item->type != CLOG_CONFIG_ITEM_TYPE_STRING, CLOG_ERROR_FORMAT,
                  "format type error, CLOG_CONFIG_ITEM_TYPE_STRING expected, but %u found", item->type);
    CLOG_RET_IF_NULL_X(context, item->value.str, CLOG_ERROR_FORMAT, "format string is NULL");

    const clog_res_e ret = clog_placeholder_parse_format(context, &context->formatter.placeholder, item->value.str);
    if (ret != CLOG_SUCCESS) {
        clog_err_append_line(context, "parse format [] failed", item->value.str);
    }
    return ret;
}
