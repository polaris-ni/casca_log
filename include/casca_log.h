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
 * @return #clog_res_e
 */
clog_res_e clog_init(const char* process, const char* config);

/**
 * get pared clog_config_group_t from config file
 * @return parsed clog_config_group_t, NULL if clog_init failed or clog_setup called
 */
const clog_config_group_t* clog_get_config_root(void);

/**
 * get current process name
 * @return process, return "NULL" if process is not set
 */
const char* clog_get_process(void);

/**
 * destroy context
 */
void clog_destroy(void);

/**
 * clear error message
 */
void clog_err_clear(void);

/**
 * set err msg, will clear previous error message even if set failed
 * @param fmt message format, nonnull
 * @param ... var
 */
void clog_err_set(const char* fmt, ...);

/**
 * append error message
 * @param fmt message format, nonnull
 * @param ... var
 */
void clog_err_append(const char* fmt, ...);

/**
 * append error message with line
 * @param fmt message format, nonnull
 * @param ... var
 */
void clog_err_append_line(const char* fmt, ...);

/**
 * get error message, it will never return NULL, safe to print
 * @return error message, return string "NULL" if context is NULL
 */
const char* clog_err_get(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_H */
