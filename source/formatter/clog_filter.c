/**
 * @author Polaris
 * @date  2025/10/12
 */

#include "clog_filter.h"
#include "casca_log.h"

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

bool clog_filter_basic(const clog_item_t* item)
{
    CLOG_RET_IF_NULL(item, false);
    const clog_module_t* info = clog_get_module_info(item->module);
    CLOG_RET_IF_NULL(info, false);
    return (item->level & info->level) != 0;
}
