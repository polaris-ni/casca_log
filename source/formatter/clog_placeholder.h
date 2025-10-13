/**
 * @author Polaris
 * @date  2025/9/19
 */
#ifndef CASCA_LOG_CLOG_PLACEHOLDER_H
#define CASCA_LOG_CLOG_PLACEHOLDER_H

#include "casca_log_base.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * placeholder
 * @param item log item, contains log info and content, it should never be NULL
 * @param buf the buffer to store log
 * @param size buffer size
 * @return size_t write size
 */
typedef size_t (*clog_placeholder_f)(const clog_item_t* item, char* buf, size_t size);

typedef struct clog_placeholder {
    const char* name;
    clog_placeholder_f func;
    const struct clog_placeholder* next;
} clog_placeholder_t;

/**
 * register customize placeholder func
 * if you register a placeholder with the same name, the previous one will be replaced
 * this function should be called before clog_init which will parse customized and default placeholders
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
