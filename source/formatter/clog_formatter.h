/**
 * @author Polaris
 * @date  2025/10/1
 */
#ifndef CASCA_LOG_CLOG_FORMATTER_H
#define CASCA_LOG_CLOG_FORMATTER_H

#include "casca_log_base.h"
#include "clog_interpolator.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * format #clog_item_t with #clog_placeholder_t
 * @param interpolator interpolator
 * @param wrapper clog item with params
 * @param content log buffer
 * @param size log buffer size
 * @param num number of log character
 * @return #clog_res_e
 */
clog_res_e clog_format_log(const clog_interpolator_t *interpolator, const clog_item_wrapper_t *wrapper, char *content,
                           size_t size, size_t *num);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_FORMATTER_H */
