/**
 * @author Polaris
 * @date  2025/11/11
 */
#ifndef CASCA_LOG_CLOG_ATOMIC_QUEUE_H
#define CASCA_LOG_CLOG_ATOMIC_QUEUE_H

#include <stdbool.h>
#include "casca_log_base.h"
#include "clog_hooks.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum clog_atomic_queue_bias {
    CLOG_ATOMIC_QUEUE_BIASED_NONE = 0, /* GC will be triggered by this handle */
    CLOG_ATOMIC_QUEUE_BIASED_ENQUEUE = 1, /* GC will be trigger by enqueue */
    CLOG_ATOMIC_QUEUE_BIASED_DEQUEUE = 2, /* GC will be trigger by dequeue */
    CLOG_ATOMIC_QUEUE_BIASED_ANY = 3, /* GC will be triggered by both enqueue and dequeue */
} clog_atomic_queue_bias_e;

typedef struct clog_atomic_queue clog_atomic_queue_t;

typedef struct clog_atomic_queue_handle* clog_atomic_queue_handle_t;

/**
 * create atomic queue
 * @param queue atomic queue
 * @param deallocator when the queue is destroyed and the queue is not empty, the deallocator will be called
 * @return #clog_res_e
 */
clog_res_e clog_atomic_queue_create(clog_atomic_queue_t** queue, clog_deallocator_f deallocator);

/**
 * get thread local atomic queue handle
 * @param queue atomic queue
 * @param bias atomic queue GC bias
 * @return #clog_atomic_queue_handle_t
 */
clog_atomic_queue_handle_t clog_atomic_queue_attach(clog_atomic_queue_t* queue, clog_atomic_queue_bias_e bias);

/**
 * enqueue data
 * @param handle atomic queue handle
 * @param ptr data pointer, it should not be free until it is dequeued
 * @return #clog_res_e
 */
clog_res_e clog_atomic_queue_enqueue(clog_atomic_queue_handle_t handle, uintptr_t ptr);

/**
 * dequeue data
 * if the len of data dequeued is greater than #size, data will be truncated and #CLOG_OVERSIZE will be return
 * @param handle atomic queue handle
 * @param data buffer to store data
 * @return #clog_res_e
 */
clog_res_e clog_atomic_queue_dequeue(clog_atomic_queue_handle_t handle, uintptr_t* data);

/**
 * check if queue is empty
 * @param handle atomic queue handle
 * @return true if queue is NULL or empty, false otherwise
 */
bool clog_atomic_queue_is_empty(clog_atomic_queue_handle_t handle);

/**
 * detach thread local atomic queue handle
 * @param handle atomic queue
 */
void clog_atomic_queue_detach(clog_atomic_queue_handle_t handle);

/**
 * destroy atomic queue
 * @param queue atomic queue to be destroyed
 * @return CLOG_SUCCESS if all nodes are released successfully, CLOG_NOT_COMPLETED if not
 */
clog_res_e clog_atomic_queue_destroy(clog_atomic_queue_t* queue);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_ATOMIC_QUEUE_H */
