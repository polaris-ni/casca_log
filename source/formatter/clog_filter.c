/**
 * @author Polaris
 * @date  2025/10/12
 */

#include "clog_filter.h"
#include "casca_log.h"
#include "clog_hooks.h"
#include "clog_secure_func.h"

/**
 * basic filter, if module is not registered or level is not allowed, false will be returned
 */
static clog_filter_res_e clog_filter_basic(const clog_item_t* item);

static clog_filter_t g_filters[] = {{"BasicFilter", 0, CLOG_FILTER_PRE, clog_filter_basic, &g_filters[1]},
                                    {"KeywordsFilter", 0, CLOG_FILTER_POST, NULL, NULL}};

clog_res_e clog_filter_register(const char* name, clog_filter_type_e type, clog_filter_f filter)
{
    clog_filter_t* tmp = (clog_filter_t*)clog_malloc(sizeof(clog_filter_t));
    CLOG_RET_IF_NULL(tmp, CLOG_NO_MEMORY);
    tmp->name = clog_strdup(name);
    if (tmp->name == NULL) {
        clog_free(tmp);
        return CLOG_NO_MEMORY;
    }
    tmp->type = type;
    tmp->filter = filter;
    tmp->priority = 0;
    const size_t index = CLOG_ARRAY_SIZE(g_filters) - 1;
    tmp->next = g_filters[index].next;
    g_filters[index].next = tmp;
    return CLOG_SUCCESS;
}

bool clog_filter_log(const clog_filter_t* filters, const clog_item_t* item)
{
    CLOG_RET_IF_NULL(item, false);
    const clog_filter_t* filter = filters;
    while (filter != NULL) {
        CLOG_RET_IF_NULL(filter->filter, false);
        const clog_filter_res_e res = filter->filter(item);
        CLOG_RET_IF(res == CLOG_FILTER_ACCEPT, true);
        CLOG_RET_IF(res == CLOG_FILTER_REJECT, false);
        filter = filter->next;
    }
    return true;
}

static clog_filter_res_e clog_filter_basic(const clog_item_t* item)
{
    CLOG_RET_IF_NULL(item, CLOG_FILTER_REJECT);
    const clog_module_t* info = clog_get_module_info(item->module);
    CLOG_RET_IF_NULL(info, CLOG_FILTER_REJECT);
    if ((item->level & info->level) != 0) {
        return CLOG_FILTER_CONTINUE;
    }
    return CLOG_FILTER_ACCEPT;
}
