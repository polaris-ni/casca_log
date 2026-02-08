/**
 * @author Polaris
 * @date  2025/11/27
 */
#ifndef CASCA_LOG_CLOG_DISPATCHER_ASYNC_THREAD_H
#define CASCA_LOG_CLOG_DISPATCHER_ASYNC_THREAD_H

#include "clog_semaphore.h"
#include "clog_atomic_types.h"
#include "clog_dispatcher.h"
#include "clog_thread.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {

#endif

typedef struct clog_dispatcher_async_thread_param {
    clog_thread_t thread;
    atomic_uintptr_t state;
    clog_sem_t *sem;
} clog_dispatcher_async_thread_param_t;

void clog_dispatcher_async_thread(clog_dispatcher_t *dispatcher);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_DISPATCHER_ASYNC_THREAD_H */
