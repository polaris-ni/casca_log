/**
 * @author Polaris
 * @date  2025/10/12
 */
#include "clog_filter.h"
#include "casca_log.h"
#include "casca_log_keywords.h"
#include "clog_error.h"
#include "clog_hooks.h"
#include "clog_secure_func.h"

static clog_filter_res_e clog_filter_keywords(const clog_item_wrapper_t *wrapper);

static clog_filter_t g_filters[] = {{CLOG_STR_KEYWORDS_FILTER, 0, CLOG_FILTER_POST, clog_filter_keywords, NULL}};

static char **g_keywords = NULL;
static uint32_t g_keywords_count = 0;

static void clog_filter_keywords_free(char **keywords, const uint32_t num)
{
    CLOG_RET_VOID_IF_NULL(keywords);
    for (uint32_t i = 0; i < num; i++) {
        CLOG_SAFE_FREE(keywords[i]);
    }
    CLOG_SAFE_FREE(keywords);
}

static clog_res_e clog_filter_keywords_get_num(const clog_config_item_t *item, uint32_t *count)
{
    uint32_t num = 0;
    const clog_config_item_t *tmp = item;
    while (tmp != NULL) {
        CLOG_RET_IF_X(tmp->type != CLOG_CONFIG_TYPE_STRING, CLOG_ERROR_FORMAT,
                      "clog keywords contains item type other than string");
        num++;
        tmp = tmp->next;
    }
    *count = num;
    return CLOG_SUCCESS;
}

static clog_res_e clog_filter_keywords_parse(const clog_config_item_t *item, char **target, const uint32_t count)
{
    const clog_config_item_t *tmp = item;
    uint32_t num = 0;
    while (tmp != NULL) {
        target[num] = clog_strdup(tmp->value.str);
        CLOG_RET_IF_NULL_X(target[num], CLOG_FAIL, "clog_strdup keyword %s failed", tmp->value.str);
        num++;
        tmp = tmp->next;
    }
    return CLOG_SUCCESS;
}

static clog_res_e clog_filter_keywords_init(const clog_config_group_t *group)
{
    const char *name[] = {CLOG_STR_KEYWORDS_FILTER};
    const clog_config_group_t *filter = clog_config_find_group(group, name, CLOG_ARRAY_SIZE(name));
    CLOG_RET_IF_NULL(filter, CLOG_SUCCESS); /* keywords filter is not configured */
    bool enabled = true;
    clog_res_e ret = clog_config_find_item_in_group_bool(filter, CLOG_STR_ENABLED, &enabled);
    if (ret != CLOG_SUCCESS) {
        /* "enabled" not found CLOG_TARGET_NOT_FOUND, use default value true, otherwise return */
        CLOG_RET_IF(ret != CLOG_TARGET_NOT_FOUND, ret);
    } else {
        CLOG_RET_IF(!enabled, CLOG_SUCCESS); /* keywords filter is not enabled */
    }

    const clog_config_item_t *keywords = NULL;
    ret = clog_config_find_item_in_group_array(filter, CLOG_STR_KEYWORDS, &keywords);
    CLOG_RET_IF_FAILED(ret); /* "keywords" is not configured */
    CLOG_RET_IF_NULL(keywords, CLOG_TARGET_NOT_FOUND); /* keywords not found */
    uint32_t num = 0;
    ret = clog_filter_keywords_get_num(keywords, &num);
    CLOG_RET_IF_FAILED(ret);
    const size_t size = num * sizeof(char *);
    char **arr = clog_malloc(size);
    CLOG_RET_IF_NULL(arr, CLOG_NO_MEMORY);
    CLOG_IGNORE_RES(clog_memset(arr, size, 0, size));
    ret = clog_filter_keywords_parse(keywords, arr, num);
    CLOG_CLEAN_RET_IF_FAILED(ret, clog_filter_keywords_free(arr, num));
    g_keywords = arr;
    g_keywords_count = num;
    return CLOG_SUCCESS;
}

clog_res_e clog_filter_register(const char *name, const clog_filter_type_e type, const clog_filter_f filter)
{
    clog_filter_t *tmp = clog_malloc(sizeof(clog_filter_t));
    CLOG_RET_IF_NULL(tmp, CLOG_NO_MEMORY);
    tmp->name = clog_strdup(name);
    if (tmp->name == NULL) {
        clog_free(tmp);
        return CLOG_NO_MEMORY;
    }
    tmp->priority = 0;
    tmp->type = type;
    tmp->filter = filter;
    const size_t index = CLOG_ARRAY_SIZE(g_filters) - 1;
    tmp->next = g_filters[index].next;
    g_filters[index].next = tmp;
    return CLOG_SUCCESS;
}

static void clog_filter_clear(clog_filter_t *filter)
{
    CLOG_SAFE_FREE(filter->name);
    filter->type = 0;
    filter->filter = NULL;
    filter->priority = 0;
    clog_filter_t *tmp = (clog_filter_t *)filter->next;
    filter->next = NULL;
    while (tmp != NULL) {
        const clog_filter_t *next = tmp->next;
        CLOG_SAFE_FREE(tmp->name);
        CLOG_SAFE_FREE(tmp);
        tmp = (clog_filter_t *)next;
    }
}

void clog_filter_free(clog_filter_t *filter)
{
    CLOG_RET_VOID_IF_NULL(filter);
    clog_filter_clear(filter);
    clog_free(filter);
}

static void clog_filter_add_filter(clog_filter_t *chain, clog_filter_t *filter)
{
    clog_filter_t *tmp = (clog_filter_t *)chain->next;
    clog_filter_t *last = chain;
    while (tmp != NULL) {
        if (filter->priority < tmp->priority) {
            break;
        }
        last = tmp;
        tmp = (clog_filter_t *)tmp->next;
    }
    filter->next = tmp;
    last->next = filter;
}

static clog_res_e clog_filter_parse(const clog_config_group_t *group, clog_filter_t *pre, clog_filter_t *post)
{
    CLOG_RET_IF_NULL(group->name, CLOG_INVALID_PARAM);
    clog_filter_t *tmp = clog_malloc(sizeof(clog_filter_t));
    CLOG_RET_IF_NULL(tmp, CLOG_NO_MEMORY);
    const clog_filter_t *filter = &g_filters[0];
    while (filter != NULL) {
        if (filter->name != NULL && strcmp(filter->name, group->name) == 0) {
            tmp->name = clog_strdup(group->name);
            CLOG_CLEAN_RET_IF_NULL(tmp, clog_filter_free(tmp), CLOG_NO_MEMORY);
            clog_res_e ret = clog_config_find_item_in_group_uint(group, CLOG_STR_PRIORITY, &tmp->priority);
            CLOG_CLEAN_RET_IF_FAILED(ret, clog_filter_free(tmp));
            uint32_t type = 0;
            ret = clog_config_find_item_in_group_uint(group, CLOG_STR_TYPE, &type);
            CLOG_CLEAN_RET_IF_FAILED(ret, clog_filter_free(tmp));
            CLOG_CLEAN_RET_IF_FAILED(type != CLOG_FILTER_PRE && type != CLOG_FILTER_POST, clog_filter_free(tmp));
            tmp->type = type;
            tmp->filter = filter->filter;
            if (tmp->type == CLOG_FILTER_PRE) {
                clog_filter_add_filter(pre, tmp);
            } else {
                clog_filter_add_filter(post, tmp);
            }
            return CLOG_SUCCESS;
        }
        filter = filter->next;
    }
    return CLOG_TARGET_NOT_FOUND;
}

clog_res_e clog_filter_setup(void)
{
    const clog_config_group_t *root = clog_get_config_root();
    CLOG_RET_IF_NULL(root, CLOG_INVALID_PARAM);
    const char *group[] = {CLOG_STR_FILTERS};
    const clog_config_group_t *filters = clog_config_find_group(root, group, CLOG_ARRAY_SIZE(group));
    CLOG_RET_IF_NULL(filters, CLOG_INVALID_PARAM);
    clog_res_e ret = clog_filter_keywords_init(filters);
    CLOG_RET_IF_FAILED(ret);

    clog_filter_t pre = {NULL, 0, CLOG_FILTER_PRE, NULL, NULL};
    clog_filter_t post = {NULL, 0, CLOG_FILTER_PRE, NULL, NULL};

    const clog_config_group_t *child = filters->child;
    while (child != NULL) {
        bool enabled = true;
        ret = clog_config_find_item_in_group_bool(root, CLOG_STR_ENABLED, &enabled);
        if (ret != CLOG_SUCCESS) {
            /* "enabled" not found CLOG_TARGET_NOT_FOUND, use default value true, otherwise return */
            CLOG_CLEAN_RET_IF(ret != CLOG_TARGET_NOT_FOUND, (clog_filter_clear(&pre), clog_filter_clear(&post)), ret);
        } else {
            if (!enabled) {
                child = child->sibling;
                continue;
            }
        }
        ret = clog_filter_parse(child, &pre, &post);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, (clog_filter_clear(&pre), clog_filter_clear(&post)),
                                   "clog_filter_parse failed, ret = %u", ret);
        child = child->sibling;
    }
    clog_set_filters(pre.next, post.next);
    return CLOG_SUCCESS;
}

void clog_filter_cleanup(void)
{
    clog_filter_free((clog_filter_t *)g_filters[CLOG_ARRAY_SIZE(g_filters) - 1].next);
    g_filters[CLOG_ARRAY_SIZE(g_filters) - 1].next = NULL;
    clog_filter_keywords_free(g_keywords, g_keywords_count);
    g_keywords = NULL;
    g_keywords_count = 0;
}

bool clog_filter_log(const clog_filter_t *filters, const clog_item_wrapper_t *wrapper)
{
    CLOG_RET_IF_NULL(wrapper, true); /* no filters, pass */
    const clog_filter_t *filter = filters;
    while (filter != NULL) {
        CLOG_RET_IF_NULL(filter->filter, false);
        const clog_filter_res_e res = filter->filter(wrapper);
        CLOG_RET_IF(res == CLOG_FILTER_ACCEPT, true);
        CLOG_RET_IF(res == CLOG_FILTER_REJECT, false);
        filter = filter->next;
    }
    return true;
}

static clog_filter_res_e clog_filter_keywords(const clog_item_wrapper_t *wrapper)
{
    CLOG_RET_IF_NULL(wrapper, CLOG_FILTER_REJECT);
    CLOG_RET_IF_NULL(g_keywords, CLOG_FILTER_REJECT);
    for (uint32_t i = 0; i < g_keywords_count; ++i) {
        const char *str = strstr(wrapper->log->content, g_keywords[i]);
        CLOG_RET_IF(str != NULL, CLOG_FILTER_REJECT);
    }
    return CLOG_FILTER_CONTINUE;
}
