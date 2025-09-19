/**
 * @auther Polaris
 * @date  2025/9/19
 */
#include "clog_placeholder.h"

#include "casca_log.h"
#include "clog_hooks.h"
#include "clog_secure_func.h"

#define CLOG_PLACEHOLDER_DECLARE(n, f, i) {.name = n, .placeholder = f, .next = &g_placeholder_list[i + 1]}

#define CLOG_PLACEHOLDER_DECLARE_LAST(n, f) {.name = n, .placeholder = f, .next = NULL}

static clog_placeholder_t g_placeholder_list[] = {
    CLOG_PLACEHOLDER_DECLARE("_date", clog_placeholder_date, 0),
    CLOG_PLACEHOLDER_DECLARE_LAST("_time", clog_placeholder_time),
};

size_t clog_placeholder_date(const clog_context_t* context, char* buf, size_t buf_size)
{
    return 0;
}

size_t clog_placeholder_time(const clog_context_t* context, char* buf, size_t buf_size)
{
    return 0;
}

clog_res_e clog_placeholder_register(clog_context_t* context, const char* name, const clog_placeholder_f func)
{
    CLOG_RET_IF_NULL(context, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(name, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(func, CLOG_INVALID_PARAM);
    if (name[0] < 'a' || name[0] > 'z' || name[0] < 'A' || name[0] > 'Z' || name[0] < '0' || name[0] > '9') {
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
    placeholder->placeholder = func;
    placeholder->next = last->next;
    last->next = placeholder;
    return CLOG_SUCCESS;
}
