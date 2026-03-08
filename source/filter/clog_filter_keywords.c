/**
 * @author Polaris
 * @date 2026/3/8
 */

#include "clog_filter_keywords.h"
#include <string.h>
#include "clog_error.h"
#include "clog_secure_func.h"

clog_filter_res_e clog_filter_keywords(const clog_filter_t *filter, const clog_item_wrapper_t *item)
{
    if (strstr(item->log->content, filter->param) != NULL) {
        return CLOG_FILTER_REJECT;
    }
    return CLOG_FILTER_CONTINUE;
}

clog_filter_t *clog_filter_keywords_create(uint32_t priority, const char *keywords)
{
    CLOG_RET_IF_NULL_X(keywords, NULL, "keywords is null");
    clog_filter_t *filter = clog_malloc(sizeof(clog_filter_t));
    CLOG_RET_IF_NULL_X(filter, NULL, "malloc clog_filter_t failed");
    filter->priority = priority;
    filter->type = CLOG_FILTER_POST;
    filter->filter = clog_filter_keywords;
    filter->free = clog_sys_free;
    filter->param = clog_strdup(keywords);
    filter->next = NULL;
    CLOG_CLEAN_RET_IF_NULL_X(filter->param, clog_filter_free(filter), NULL, "clog_strdup keywords %s failed", keywords);
    return filter;
}
