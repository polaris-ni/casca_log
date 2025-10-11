/**
 * @author Polaris
 * @date  2025/10/12
 */

#include "clog_filter.h"

bool clog_do_filter(const clog_filter_t* filters, const clog_item_t* item)
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
