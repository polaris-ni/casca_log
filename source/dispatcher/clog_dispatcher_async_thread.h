/**
 * @author Polaris
 * @date  2025/11/27
 */
#ifndef CASCA_LOG_CLOG_DISPATCHER_ASYNC_THREAD_H
#define CASCA_LOG_CLOG_DISPATCHER_ASYNC_THREAD_H

#include "clog_dispatcher.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * create async thread dispatcher
 * @return #clog_dispatcher_t
 */
clog_dispatcher_t *clog_dispatcher_async_thread_create();

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_DISPATCHER_ASYNC_THREAD_H */
