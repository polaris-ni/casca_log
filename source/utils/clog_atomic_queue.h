/**
 * @author Polaris
 * @date  2025/11/11
 */
#ifndef CASCA_LOG_CLOG_ATOMIC_QUEUE_H
#define CASCA_LOG_CLOG_ATOMIC_QUEUE_H

#include <stdbool.h>
#include "casca_log_base.h"

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
 * @param item_size the size of item in queue
 * @param threshold when usage is lower than threshold, queue will be shrinking
 * @return #clog_res_e
 */
clog_res_e clog_atomic_queue_create(clog_atomic_queue_t** queue, size_t item_size, uint8_t threshold);

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
 * @param data data
 * @param size size of #data
 * @return #clog_res_e
 */
clog_res_e clog_atomic_queue_enqueue(clog_atomic_queue_handle_t handle, const void* data, size_t size);

/**
 * dequeue data
 * if the len of data dequeued is greater than #size, data will be truncated and #CLOG_OVERSIZE will be return
 * @param handle atomic queue handle
 * @param data buffer to store data, if it is NULL, dequeue will still be performed
 * @param size size of #data
 * @param len nullable, it will be set to the actual size of the extracted data (even if truncation occurs) if not null
 * @return #clog_res_e
 */
clog_res_e clog_atomic_queue_dequeue(clog_atomic_queue_handle_t handle, void* data, size_t size, size_t* len);

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
