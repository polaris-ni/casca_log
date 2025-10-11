/**
 * @author Polaris
 * @date  2025/10/1
 */
#ifndef CASCA_LOG_CLOG_FORMATTER_H
#define CASCA_LOG_CLOG_FORMATTER_H

#include "casca_log_base.h"
#include "clog_placeholder.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * formatter setup, init placeholders
 * @return clog_res_e
 */
clog_res_e clog_formatter_setup(void);

/**
 * format #clog_item_t with #clog_placeholder_t
 * @param placeholders placeholders
 * @param item clog item with params
 * @param content log buffer
 * @param size log buffer size
 * @return #clog_res_e
 */
clog_res_e clog_format_log(const clog_placeholder_t* placeholders, const clog_item_t* item, char* content, size_t size);

/**
 * cleanup formatter resource
 */
void clog_formatter_cleanup(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_FORMATTER_H */
