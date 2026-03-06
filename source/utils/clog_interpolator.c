/**
 * @author Polaris
 * @date 2026/2/23
 */

#include "clog_interpolator.h"
#include "clog_error.h"

struct clog_interpolator_context {
    bool is_map_allocated; /* whether the map is allocated when context create */
    clog_hashmap_t *map; /* KV: name -> clog_placeholder_handler_f */
};

struct clog_interpolator {
    union {
        struct {
            char *string;
            size_t length;
        } pure;

        clog_placeholder_handler_f handler;
    };

    bool should_handle;
    clog_interpolator_t *next;
};

clog_interpolator_context_t *clog_interpolator_context_create(clog_hashmap_t *map)
{
    clog_interpolator_context_t *context = clog_malloc(sizeof(clog_interpolator_context_t));
    CLOG_RET_IF_NULL_X(context, NULL, "malloc clog_interpolator_context_t failed");
    if (map != NULL) {
        context->map = map;
        context->is_map_allocated = false;
    } else {
        context->map = clog_hashmap_create(0, sizeof(clog_placeholder_handler_f), clog_hashmap_string_dup,
                                           clog_hashmap_string_free, NULL, NULL, clog_hashmap_string_cmp,
                                           clog_hashmap_string_size, 0);
        CLOG_CLEAN_RET_IF_NULL_X(context->map, clog_free(context), NULL, "clog_hashmap_create failed");
        context->is_map_allocated = true;
    }
    return context;
}

clog_res_e clog_interpolator_context_register(clog_interpolator_context_t *context, const char *name,
                                              clog_placeholder_handler_f handler)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_RET_IF_NULL_X(name, CLOG_INVALID_PARAM, "name is NULL");
    CLOG_RET_IF_NULL_X(handler, CLOG_INVALID_PARAM, "handler is NULL");
    return clog_hashmap_put(context->map, name, &handler);
}

void clog_interpolator_context_destroy(clog_interpolator_context_t **context)
{
    CLOG_RET_VOID_IF_NULL_X(context, "context is NULL");
    CLOG_RET_VOID_IF_NULL_X(*context, "*context is NULL");
    if ((*context)->is_map_allocated) {
        clog_hashmap_destroy(&((*context)->map));
    }
    clog_free(*context);
    *context = NULL;
}

static bool clog_placeholder_is_name_valid(const char ch)
{
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_') {
        return true;
    }
    return false;
}

static clog_interpolator_t *clog_interpolator_create(const clog_hashmap_t *map, const char *name, bool should_handle)
{
    const clog_placeholder_handler_f *func = NULL;
    if (should_handle) {
        func = clog_hashmap_get(map, name);
        CLOG_RET_IF_NULL_X(func, NULL, "get %s function failed", name);
    }

    clog_interpolator_t *interpolator = clog_malloc(sizeof(clog_interpolator_t));
    CLOG_RET_IF_NULL(interpolator, NULL);

    if (!should_handle) {
        interpolator->should_handle = false;
        interpolator->pure.string = clog_strdup(name);
        CLOG_CLEAN_RET_IF_NULL_X(interpolator->pure.string, clog_free(interpolator), NULL, "clog_strdup %s failed",
                                 name);
        interpolator->pure.length = strlen(interpolator->pure.string);
    } else {
        interpolator->should_handle = true;
        interpolator->handler = *func;
    }
    interpolator->next = NULL;
    return interpolator;
}

/* {hello} -> hello */
static const char *clog_interpolator_parse_placeholder_name(const char *format)
{
    const char *tmp = format + 1;
    if (*tmp == '}') {
        CLOG_ERR_ADD("placeholder name empty {}");
        return NULL;
    }

    while (*tmp != '\0') {
        if (*tmp == '}') {
            return tmp - 1;
        }
        if (!clog_placeholder_is_name_valid(*tmp)) {
            CLOG_ERR_ADD("placeholder contains invalid char [%c]", *tmp);
            return NULL;
        }
        tmp++;
    }
    CLOG_ERR_ADD("placeholder does not contain '}'");
    return NULL;
}

static clog_res_e clog_interpolator_parse_format(const clog_hashmap_t *map, const char *format,
                                                 clog_interpolator_t *root)
{
    CLOG_RET_IF(format[0] == '\0', CLOG_SUCCESS); /* parse over */
    const char *tmp = format;
    if (*tmp == '{') {
        tmp = clog_interpolator_parse_placeholder_name(format);
        CLOG_RET_IF_NULL(tmp, CLOG_INVALID_PARAM);
        char *name = clog_strndup(format + 1, tmp - format);
        CLOG_RET_IF_NULL(name, CLOG_NO_MEMORY);
        clog_interpolator_t *interpolator = clog_interpolator_create(map, name, true);
        CLOG_CLEAN_RET_IF_NULL_X(interpolator, clog_free(name), CLOG_FAIL, "clog_interpolator_create %s failed", name);
        CLOG_SAFE_FREE(name);
        root->next = interpolator;
        return clog_interpolator_parse_format(map, tmp + 2, interpolator);
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
                CLOG_ERR_ADD("there is no corresponding '{' for '}'");
                return false;
            }
        }
        is_escape = *tmp == '\\';
        tmp++;
    }
    char *name = clog_malloc(tmp - format + 1);
    char *ptr = name;
    CLOG_RET_IF_NULL(name, CLOG_NO_MEMORY);
    const char *start = format;
    while (start < tmp) {
        if (*start == '\\') {
            if (*(start + 1) == '{' || *(start + 1) == '}') {
                start++;
                continue;
            }
        }
        *ptr = *start;
        ptr++;
        start++;
    }
    *ptr = '\0';
    clog_interpolator_t *interpolator = clog_interpolator_create(map, name, false);
    CLOG_CLEAN_RET_IF_NULL_X(interpolator, clog_free(name), CLOG_FAIL, "clog_interpolator_create %s failed", name);
    CLOG_SAFE_FREE(name);
    root->next = interpolator;
    return clog_interpolator_parse_format(map, tmp, interpolator);
}

clog_res_e clog_interpolator_parse(const clog_interpolator_context_t *context, const char *fmt,
                                   clog_interpolator_t **interpolator)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_RET_IF_NULL_X(fmt, CLOG_INVALID_PARAM, "interpolator string is NULL");
    CLOG_RET_IF_NULL_X(interpolator, CLOG_INVALID_PARAM, "interpolator is NULL");

    clog_interpolator_t *root = clog_malloc(sizeof(clog_interpolator_t));
    CLOG_RET_IF_NULL_X(root, CLOG_NO_MEMORY, "malloc root clog_interpolator_t failed");
    const clog_res_e ret = clog_interpolator_parse_format(context->map, fmt, root);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_interpolator_clear(&root), "parse fmt %s failed, ret = %u", fmt, ret);
    *interpolator = root;
    return CLOG_SUCCESS;
}

clog_res_e clog_interpolator_interpolate(const clog_interpolator_t *interpolator, void *param, char *buf, size_t size,
                                         size_t *num)
{
    CLOG_RET_IF_NULL_X(interpolator, CLOG_INVALID_PARAM, "interpolator is NULL");
    CLOG_RET_IF_NULL_X(buf, CLOG_INVALID_PARAM, "buf is NULL");
    CLOG_RET_IF_X(size <= 1, CLOG_INVALID_PARAM, "size is too small");
    const clog_interpolator_t *tmp = interpolator->next;
    size_t offset = 0;
    clog_res_e res;
    while (tmp != NULL) {
        if (!tmp->should_handle) {
            CLOG_ASSERT(tmp->pure.string != NULL);
            CLOG_ASSERT(tmp->pure.length > 0);
            res = clog_strcpy(buf + offset, size - offset, tmp->pure.string);
            if (res != CLOG_SUCCESS) {
                CLOG_ERR_ADD("clog_interpolator_interpolate copy string %s failed, offset = %zu, size = %zu, ret = %u",
                             tmp->pure.string, offset, size, res);
                buf[0] = '\0';
                return res;
            }
            offset += tmp->pure.length;
        } else {
            CLOG_ASSERT(tmp->handler != NULL);
            const int len = tmp->handler(param, buf + offset, size - offset);
            if (len <= 0) {
                res = -len;
                CLOG_ERR_ADD("clog_interpolator_interpolate func failed, offset = %zu, size = %zu, ret = %u", offset,
                             size, res);
                buf[0] = '\0';
                return res;
            }
            offset += len;
        }
        tmp = tmp->next;
    }
    if (offset >= size) {
        CLOG_ERR_ADD("clog_interpolator_interpolate buf overflow, offset = %zu, size = %u", offset, size);
        buf[0] = '\0';
        return CLOG_NO_MEMORY;
    }
    buf[offset] = '\0';
    if (num != NULL) {
        *num = offset;
    }
    return CLOG_SUCCESS;
}

void clog_interpolator_clear(clog_interpolator_t **interpolator)
{
    CLOG_RET_VOID_IF_NULL_X(interpolator, "interpolator is NULL");
    CLOG_RET_VOID_IF_NULL_X(*interpolator, "interpolator is NULL");
    clog_interpolator_t *head = (*interpolator)->next;
    while (head != NULL) {
        clog_interpolator_t *next = head->next;
        if (!head->should_handle) {
            clog_free(head->pure.string);
        }
        clog_free(head);
        head = next;
    }
    clog_free(*interpolator);
    *interpolator = NULL;
}
