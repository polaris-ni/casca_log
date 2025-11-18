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

typedef struct clog_atomic_queue clog_atomic_queue_t;

/**
 * create atomic queue
 * @param queue atomic queue
 * @param item_size the size of item in queue
 * @param threshold when usage is lower than threshold, queue will be shrinking
 * @return #clog_res_e
 */
clog_res_e clog_atomic_queue_create(clog_atomic_queue_t** queue, size_t item_size, uint8_t threshold);

/**
 * enqueue data
 * @param queue atomic queue
 * @param data data
 * @param size size of #data
 * @return #clog_res_e
 */
clog_res_e clog_atomic_queue_enqueue(clog_atomic_queue_t* queue, const void* data, size_t size);

/**
 * dequeue data
 * if the len of data dequeued is greater than #size, data will be truncated and #CLOG_OVERSIZE will be return
 * @param queue atomic queue
 * @param data buffer to store data, if it is NULL, dequeue will still be performed
 * @param size size of #data
 * @param len nullable, it will be set to the actual size of the extracted data (even if truncation occurs) if not null
 * @return #clog_res_e
 */
clog_res_e clog_atomic_queue_dequeue(clog_atomic_queue_t* queue, void* data, size_t size, size_t* len);

/**
 * check if queue is empty
 * @param queue atomic queue
 * @return true if queue is NULL or empty, false otherwise
 */
bool clog_atomic_queue_is_empty(const clog_atomic_queue_t* queue);

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
