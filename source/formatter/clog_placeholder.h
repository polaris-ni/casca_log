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

/**
 * register customize placeholder func
 * if you register a placeholder with the same name, the previous one will be replaced
 * @param name placeholder name, should start with [a-z, A-Z, 0-9]
 * @param func placeholder func
 * @return #clog_res_e
 */
clog_res_e clog_placeholder_register(const char* name, clog_placeholder_f func);

/**
 * parse "Formatter.format" to placeholder function
 * @param format log context
 * @param root root node, parsed results will be added to #next of root, so #next of root shall be NULL
 * @return #clog_res_e
 */
clog_res_e clog_placeholder_parse(const char* format, clog_placeholder_t* root);

/**
 * recursively clean up all child nodes contained in root, but root itself will not be freed.
 * @param root placeholder to be clear
 */
void clog_placeholder_clear(clog_placeholder_t* root);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_PLACEHOLDER_H */
