/**
 * @author Polaris
 * @date  2025/10/12
 */
#include "clog_filter.h"
#include "clog_hooks.h"
#include "clog_secure_func.h"

void clog_filter_free(clog_filter_t *filter)
{
    clog_filter_t *head = filter;
    while (head != NULL) {
        if (filter->param != NULL) {
            if (filter->free != NULL) {
                filter->free(filter->param);
            } else {
                clog_free(filter->param);
            }
            filter->param = NULL;
        }
        clog_filter_t *tmp = (clog_filter_t *)head->next;
        clog_free(head);
        head = tmp;
    }
}

bool clog_filter_log(const clog_filter_t *filters, const clog_item_wrapper_t *wrapper)
{
    CLOG_RET_IF_NULL(wrapper, true); /* no filters, pass */
    const clog_filter_t *filter = filters;
    while (filter != NULL) {
        CLOG_RET_IF_NULL(filter->filter, false);
        const clog_filter_res_e res = filter->filter(filter, wrapper);
        CLOG_RET_IF(res == CLOG_FILTER_ACCEPT, true);
        CLOG_RET_IF(res == CLOG_FILTER_REJECT, false);
        filter = filter->next;
    }
    return true;
}
