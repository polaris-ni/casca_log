/**
 * @author polaris
 * @date  2025/11/28
 */
#ifndef CASCA_LOG_CLOG_ATOMIC_MPSC_QUEUE_H
#define CASCA_LOG_CLOG_ATOMIC_MPSC_QUEUE_H

#include "casca_log_base.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct clog_atomic_mpsc_queue clog_atomic_mpsc_queue_t;

/**
 * create a multi producer and single consumer queue
 * @return #clog_atomic_mpsc_queue_t
 */
clog_atomic_mpsc_queue_t* clog_atomic_mpsc_queue_create(void);

/**
 * data enqueue
 * @param queue #clog_atomic_mpsc_queue_t
 * @param data raw data or ptr
 * @return #clog_res_e
 */
clog_res_e clog_atomic_mpsc_queue_in(clog_atomic_mpsc_queue_t* queue, uintptr_t data);

/**
 * data dequeue
 * @param queue #clog_atomic_mpsc_queue_t
 * @param data ptr to store dequeued data
 * @return #clog_res_e
 */
clog_res_e clog_atomic_mpsc_queue_out(clog_atomic_mpsc_queue_t* queue, uintptr_t* data);

/**
 * destroy queue, ensure it will never be used
 * @param queue
 */
void clog_atomic_mpsc_queue_destroy(clog_atomic_mpsc_queue_t* queue);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_ATOMIC_MPSC_QUEUE_H */
