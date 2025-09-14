/**
 * @auther Polaris
 * @date  2025/9/2
 */
#ifndef CASCA_LOG_CASCA_LOG_H
#define CASCA_LOG_CASCA_LOG_H

#include "casca_log_config.h"
#include "casca_log_defines.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifdef CASCA_LOG_DEBUG
/* suppress warning: Possibly unused #include directive */
#endif

clog_context_t* clog_create(const char* process);

void clog_destroy(clog_context_t* context);

/**
 * clear error message
 * @param context context
 */
void clog_err_clear(clog_context_t* context);

/**
 * set err msg, will clear previous error message even if set failed
 * @param context context to be set msg
 * @param fmt message format
 * @param ... var
 */
void clog_err_set(clog_context_t* context, const char* fmt, ...);

/**
 * get error message, never return NULL, safe to print
 * @param context context
 * @return error message, return string "NULL" if context is NULL
 */
const char* clog_err_get(const clog_context_t* context);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_H */
