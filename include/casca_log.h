/**
 * @author Polaris
 * @date  2025/9/2
 */
#ifndef CASCA_LOG_CASCA_LOG_H
#define CASCA_LOG_CASCA_LOG_H

#include "clog_buffer_pool.h"
#include "clog_config.h"
#include "clog_filter.h"
#include "clog_placeholder.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * setup function
 * @return #clog_res_e
 */
typedef clog_res_e (*clog_setup_f)(void);

/**
 * setup function
 * @return #clog_res_e
 */
typedef void (*clog_cleanup_f)(void);

typedef struct clog_module {
    clog_level_e level;
    uint32_t num;
    uint32_t recorders[CASCA_LOG_TARGET_RECORDER_MAX_NUM]; /*  */
} clog_module_t;

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
 * get pre-filters
 * @param type #clog_filter_type_e
 * @return #clog_filter_t, NULL if there is no filter
 */
const clog_filter_t* clog_get_filters(clog_filter_type_e type);

/**
 * set prefilter and postfilter
 * @param pre prefilter chain
 * @param post postfilter chain
 */
void clog_set_filters(const clog_filter_t* pre, const clog_filter_t* post);

/**
 * get module config
 * @param module module name
 * @return #clog_module_t, NULL if module is not existed or not enabled
 */
const clog_module_t* clog_get_module_info(const char* module);

/**
 * get log buffer pool
 * @return #clog_buffer_pool_t
 */
clog_buffer_pool_t* clog_get_buffer_pool(void);

/**
 * destroy clog
 * @param funcs customized cleanup functions, NULL if there is no customized cleanup function
 * @param num number of funcs, 0 if there is no customized cleanup function
 */
void clog_destroy(const clog_cleanup_f* funcs, size_t num);

/**
 * record a log
 * @param module module of current process
 * @param recorder where the log will be recorded
 * @param file current file name
 * @param function current function name
 * @param line current line number
 * @param level log level
 * @param fmt message format
 * @param ... var args
 * @return #clog_res_e
 */
clog_res_e clog_log(const char* module, uint32_t recorder, const char* file, const char* function, int line,
                    clog_level_e level, const char* fmt, ...);

/**
 * record a log using recoders that configured
 * @param module module of current process
 * @param file current file name
 * @param function current function name
 * @param line current line number
 * @param level log level
 * @param fmt message format
 * @param ... var args
 * @return #clog_res_e
 */
clog_res_e clog_module_log(const char* module, const char* file, const char* function, int line, clog_level_e level,
                           const char* fmt, ...);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_H */
