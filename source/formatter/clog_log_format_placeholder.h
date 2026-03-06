/**
 * @author Polaris
 * @date  2025/9/19
 */
#ifndef CASCA_LOG_CLOG_LOG_FORMAT_PLACEHOLDER_H
#define CASCA_LOG_CLOG_LOG_FORMAT_PLACEHOLDER_H

#include "clog_interpolator.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * register customize log format placeholder handler
 * if you register a placeholder with the same name, the previous one will be replaced
 * this function should be called before clog_init which will parse customized and default placeholders
 * @param name placeholder name, should start with [a-z, A-Z, 0-9]
 * @param handler placeholder handler function
 * @return #clog_res_e
 */
clog_res_e clog_log_format_placeholder_register(const char *name, clog_placeholder_handler_f handler);

/**
 * create log format default placeholder map
 * @return log format placeholder map
 */
clog_hashmap_t *clog_log_format_default_placeholder_map_create(void);

/**
 * get log format placeholder map
 * @return log format placeholder map
 */
clog_hashmap_t *clog_log_format_placeholder_get_map(void);

/**
 * clear log format placeholder map
 */
void clog_log_format_placeholder_clear(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_LOG_FORMAT_PLACEHOLDER_H */
