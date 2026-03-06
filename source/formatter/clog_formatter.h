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
 * formatter setup, init placeholders
 * @return clog_res_e
 */
clog_res_e clog_formatter_setup(void);

/**
 * parse log format to interpolator
 * @param format log format
 * @param interpolator parsed interpolator
 * @return #clog_res_e
 */
clog_res_e clog_formatter_parse(const char *format, clog_interpolator_t **interpolator);

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

/**
 * cleanup formatter resource
 */
void clog_formatter_cleanup(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_FORMATTER_H */
