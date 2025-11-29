/**
 * @author Polaris
 * @date  2025/11/27
 */
#ifndef CASCA_LOG_CLOG_DISPATCHER_ASYNC_THREAD_H
#define CASCA_LOG_CLOG_DISPATCHER_ASYNC_THREAD_H
#include "clog_atomic_mpsc_queue.h"
#include "clog_buffer_pool.h"
#include "clog_dispatcher.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct clog_dispatcher_async_thread_param {
    clog_atomic_mpsc_queue_t* queue;
    clog_buffer_pool_t* pool;
} clog_dispatcher_async_thread_param_t;

void clog_dispatcher_async_thread(clog_dispatcher_t *dispatcher);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_DISPATCHER_ASYNC_THREAD_H */
