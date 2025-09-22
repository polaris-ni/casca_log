/**
 * @author Polaris
 * @date  2025/9/19
 */
#ifndef CASCA_LOG_CLOG_PLACEHOLDER_H
#define CASCA_LOG_CLOG_PLACEHOLDER_H

#include "casca_log_defines.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

size_t clog_placeholder_date(const clog_context_t* context, char* buf, size_t buf_size);

size_t clog_placeholder_time(const clog_context_t* context, char* buf, size_t buf_size);

/**
 * register customize placeholder func
 * if you register a placeholder with the same name, the previous one will be replaced
 * @param context context
 * @param name placeholder name, should start with [a-z, A-Z, 0-9]
 * @param func placeholder func
 * @return #clog_res_e
 */
clog_res_e clog_placeholder_register(clog_context_t* context, const char* name, clog_placeholder_f func);

/**
 * parse "Formatter.format" to placeholder function
 * @param context log context
 * @return #clog_res_e
 */
clog_res_e clog_placeholder_parse(clog_context_t* context);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_PLACEHOLDER_H */
