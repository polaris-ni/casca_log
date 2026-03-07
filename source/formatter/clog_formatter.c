/**
 * @auther Polaris
 * @date  2025/10/1
 */
#include "clog_formatter.h"
#include "clog_config.h"
#include "clog_log_format_placeholder.h"
#include "clog_secure_func.h"

clog_res_e clog_format_log(const clog_interpolator_t *interpolator, const clog_item_wrapper_t *wrapper, char *content,
                           size_t size, size_t *num)
{
    CLOG_RET_IF_NULL(interpolator, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(wrapper, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(content, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(num, CLOG_INVALID_PARAM);
    CLOG_RET_IF(size <= 1, CLOG_OVERSIZE);
    CLOG_IGNORE_RES(clog_strcpy(wrapper->log->process, sizeof(wrapper->log->process), wrapper->process));
    CLOG_IGNORE_RES(clog_strcpy(wrapper->log->module, sizeof(wrapper->log->module), wrapper->module));
    CLOG_IGNORE_RES(clog_strcpy(wrapper->log->filename, sizeof(wrapper->log->filename), wrapper->filename));

    return clog_interpolator_interpolate(interpolator, (void *)wrapper, content, size, num);
}
