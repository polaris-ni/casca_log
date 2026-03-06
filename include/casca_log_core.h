/**
 * @author Polaris
 * @date 2026/3/4
 */

#ifndef CASCA_LOG_CASCA_LOG_CORE_H
#define CASCA_LOG_CASCA_LOG_CORE_H

#include <stdbool.h>
#include "casca_log_base.h"
#include "clog_channel_base.h"
#include "clog_dispatcher.h"
#include "clog_filter.h"
#include "clog_interpolator.h"
#include "clog_recorder.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum clog_state { CLOG_STATE_CREATED, CLOG_STATE_RUNNING, CLOG_STATE_DESTROYED } clog_state_e;

typedef struct clog_context clog_context_t;

/**
 * create log context for process
 * @param process current process name
 * @param context when create success, *context will be set
 * @return pointer clog_context_t
 */
clog_res_e clog_context_create(const char *process, clog_context_t **context);

/**
 * set log mask for log context, if level & mask == 0, log will be abandoned
 * @param context clog_context_t
 * @param mask log mask
 */
void clog_set_mask(clog_context_t *context, uint32_t mask);

/**
 * setup buffer pool for log context
 * @param context clog_context_t
 * @param auto_expand whether to auto expand buffer pool
 * @param capacity buffer pool capacity
 * @param threshold buffer pool threshold
 * @return #clog_res_e
 */
clog_res_e clog_set_buffer_pool(clog_context_t *context, bool auto_expand, size_t capacity, size_t threshold);

/**
 * setup modules for log context
 * @param context clog_context_t
 * @param config modules config, using TOML format
 * @return #clog_res_e
 */
clog_res_e clog_set_modules(clog_context_t *context, const char *config);

/**
 * add module for log context
 * @param context clog_context_t
 * @param module module to be added
 * @return #clog_res_e
 */
clog_res_e clog_add_module(clog_context_t *context, const clog_module_t *module);

/**
 * add modules for log context
 * @param context clog_context_t
 * @param module modules to be added
 * @param num the number of modules
 * @return #clog_res_e
 */
clog_res_e clog_add_modules(clog_context_t *context, const clog_module_t *module, size_t num);

/**
 * register customized log format placeholder handler for log context
 * @param context clog_context_t
 * @param name placeholder name
 * @param handler placeholder handler
 * @return #clog_res_e
 */
clog_res_e clog_register_log_format_placeholder_handler(clog_context_t *context, const char *name,
                                                        clog_placeholder_handler_f handler);

/**
 * setup log format for log context
 * @param context clog_context_t
 * @param format log format
 * @return #clog_res_e
 */
clog_res_e clog_set_log_format(clog_context_t *context, const char *format);

/**
 * setup level tags for log context, default is "T", "I", "D", "W", "E", "F"
 * @param context clog_context_t
 * @param level log level
 * @param tag tag of log level
 * @return #clog_res_e
 */
clog_res_e clog_set_level_tags(clog_context_t *context, clog_level_e level, const char *tag);

/**
 * add filter for log context
 * @param context clog_context_t
 * @param filter filter to be added
 * @return #clog_res_e
 */
clog_res_e clog_add_filter(clog_context_t *context, clog_filter_t *filter);

/**
 * setup channel for log context
 * it will be freed when context is destroyed, so never free it manually if clog_set_channel success
 * if you have set a channel, it will be replaced by new channel, old channel will be freed
 * @param context clog_context_t
 * @param channel channel to be used
 * @return #clog_res_e
 */
clog_res_e clog_set_channel(clog_context_t *context, clog_channel_t *channel);

/**
 * setup dispatcher for log context, should be called after clog_set_channel
 * dispatcher->channel should be NULL, and it will be set to current configured channel automatically
 * it will be freed when context is destroyed, so never free it manually if clog_set_dispatcher success
 * if you have set a dispatcher, it will be replaced by new dispatcher, old dispatcher will be freed
 * @param context clog_context_t
 * @param dispatcher dispatcher to be used
 * @return #clog_res_e
 */
clog_res_e clog_set_dispatcher(clog_context_t *context, clog_dispatcher_t *dispatcher);

/**
 * add recorder for log context
 * recorder->extra will be assigned to new allocated clog_recorder_t
 * it will be freed when recorder->close called, so never free it manually
 * @param context clog_context_t
 * @param recorder recorder to be added
 * @return #clog_res_e
 */
clog_res_e clog_add_recorder(clog_context_t *context, const clog_recorder_t *recorder);

/**
 * open all components and start logger, it must be called before using casca log
 * @param context clog_context_t
 * @return #clog_res_e
 */
clog_res_e clog_context_setup(clog_context_t *context);

/**
 * destroy log context
 * @param context *context will be set to NULL after *context is destroyed
 */
void clog_context_destroy(clog_context_t **context);

/**
 * record a log
 * @param context clog_context_t
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
clog_res_e clog_record_log(clog_context_t *context, const char *module, uint32_t recorder, const char *file,
                           const char *function, int line, clog_level_e level, const char *fmt, ...);

/**
 * record a log using recoders that configured
 * @param context clog_context_t
 * @param module module of current process
 * @param file current file name
 * @param function current function name
 * @param line current line number
 * @param level log level
 * @param fmt message format
 * @param ... var args
 * @return #clog_res_e
 */
clog_res_e clog_module_log(clog_context_t *context, const char *module, const char *file, const char *function,
                           int line, clog_level_e level, const char *fmt, ...);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_CORE_H */
