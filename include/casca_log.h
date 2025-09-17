/**
 * @author Polaris
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

/**
 * create context with config
 * @param process process name, will be copied to context, nonnull
 * @param config config string, nonnull
 * @param err buffer to save error message, nullable
 * @param size size of err buffer
 * @return #clog_context_t, NULL if failed
 */
clog_context_t* clog_create(const char* process, const char* config, char* err, size_t size);

/**
 * destroy context
 * @param context context to be destroyed, nonnull
 */
void clog_destroy(clog_context_t* context);

/**
 * clear error message
 * @param context context, nonnull
 */
void clog_err_clear(clog_context_t* context);

/**
 * set err msg, will clear previous error message even if set failed
 * @param context context to be set msg, nonnull
 * @param fmt message format, nonnull
 * @param ... var
 */
void clog_err_set(clog_context_t* context, const char* fmt, ...);

/**
 * append error message
 * @param context context to be appended msg, nonnull
 * @param fmt message format, nonnull
 * @param ... var
 */
void clog_err_append(clog_context_t* context, const char* fmt, ...);

/**
 * append error message with line
 * @param context context to be appended msg, nonnull
 * @param fmt message format, nonnull
 * @param ... var
 */
void clog_err_append_line(clog_context_t* context, const char* fmt, ...);

/**
 * get error message, it will never return NULL, safe to print
 * @param context context, nonnull
 * @return error message, return string "NULL" if context is NULL
 */
const char* clog_err_get(const clog_context_t* context);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_H */
