/**
 * @author Polaris
 * @date  2025/10/12
 */
#include "clog_filter.h"
#include "clog_hooks.h"
#include "clog_secure_func.h"

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
