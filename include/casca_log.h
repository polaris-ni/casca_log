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
 * setup clog, submodule's setup functions will be called, must be called after clog_init
 * @param funcs customized setup functions, NULL if there is no customized setup function
 * @param num number of funcs, 0 if there is no customized setup function
 * @return #clog_res_e
 */
clog_res_e clog_setup(const clog_setup_f* funcs, size_t num);

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
 * get tag by level
 * @param level log level
 * @return tag
 */
const char* clog_get_level_tag(clog_level_e level);

/**
 * set tag by level
 * @param level log level
 * @param tag tag
 */
void clog_set_level_tag(clog_level_e level, const char* tag);

/**
 * get placeholders
 * @return placeholders
 */
const clog_placeholder_t* clog_get_placeholders(void);

/**
 * set placeholder to context, attention that placeholder will not be copy in a new memory
 * @param placeholder placeholder
 */
void clog_set_placeholders(const clog_placeholder_t* placeholder);

/**
 * destroy clog
 * @param funcs customized cleanup functions, NULL if there is no customized cleanup function
 * @param num number of funcs, 0 if there is no customized cleanup function
 */
void clog_destroy(const clog_cleanup_f* funcs, size_t num);

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

/**
 * record a log
 * @param recorders where the log will be recorded
 * @param count num of recorders, should not more than CASCA_LOG_TARGET_RECORDER_COUNT
 * @param module module of current process
 * @param file current file name
 * @param function current function name
 * @param line current line number
 * @param level log level
 * @param fmt message format
 * @param ... var args
 * @return #clog_res_e
 */
clog_res_e clog_log(const uint32_t* recorders, size_t count, const char* module, const char* file, const char* function,
                    int line, clog_level_e level, const char* fmt, ...);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_H */
