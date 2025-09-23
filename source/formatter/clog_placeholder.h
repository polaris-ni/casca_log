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

size_t clog_placeholder_year(const clog_context_t* ctx, const clog_item_t* item, char* buf, size_t size);
size_t clog_placeholder_month(const clog_context_t* ctx, const clog_item_t* item, char* buf, size_t size);
size_t clog_placeholder_day(const clog_context_t* ctx, const clog_item_t* item, char* buf, size_t size);
size_t clog_placeholder_hour(const clog_context_t* ctx, const clog_item_t* item, char* buf, size_t size);
size_t clog_placeholder_minute(const clog_context_t* ctx, const clog_item_t* item, char* buf, size_t size);
size_t clog_placeholder_second(const clog_context_t* ctx, const clog_item_t* item, char* buf, size_t size);
size_t clog_placeholder_millisecond(const clog_context_t* ctx, const clog_item_t* item, char* buf, size_t size);

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
