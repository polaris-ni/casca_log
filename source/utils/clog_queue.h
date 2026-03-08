/**
 * @author Polaris
 * @date  2026/03/08
 */
#ifndef CASCA_LOG_CLOG_QUEUE_H
#define CASCA_LOG_CLOG_QUEUE_H

#include <stdbool.h>
#include "casca_log_base.h"
#include "clog_hooks.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct clog_queue clog_queue_t;

/**
 * create a thread-safe queue
 * @param queue output parameter for the created queue
 * @param deallocator deallocator function to free queue elements, NULL if not needed
 * @return CLOG_SUCCESS on success, error code otherwise
 */
clog_res_e clog_queue_create(clog_queue_t **queue, clog_deallocator_f deallocator);

/**
 * enqueue an element to the queue
 * @param queue the queue
 * @param ptr the pointer to enqueue, it should not be freed until dequeued
 * @return CLOG_SUCCESS on success, error code otherwise
 */
clog_res_e clog_queue_enqueue(clog_queue_t *queue, uintptr_t ptr);

/**
 * dequeue an element from the queue
 * @param queue the queue
 * @param data output parameter for the dequeued element
 * @return CLOG_SUCCESS on success, CLOG_TARGET_NOT_FOUND if queue is empty, error code otherwise
 */
clog_res_e clog_queue_dequeue(clog_queue_t *queue, uintptr_t *data);

/**
 * peek the front element of the queue without removing it
 * @param queue the queue
 * @param data output parameter for the front element
 * @return CLOG_SUCCESS on success, CLOG_TARGET_NOT_FOUND if queue is empty, error code otherwise
 */
clog_res_e clog_queue_peek(const clog_queue_t *queue, uintptr_t *data);

/**
 * check if the queue is empty
 * @param queue the queue
 * @return true if queue is empty or NULL, false otherwise
 */
bool clog_queue_is_empty(const clog_queue_t *queue);

/**
 * get the current size of the queue
 * @param queue the queue
 * @return number of elements in the queue, 0 if queue is NULL
 */
size_t clog_queue_size(const clog_queue_t *queue);

/**
 * clear all elements in the queue
 * @param queue the queue
 * @return CLOG_SUCCESS on success, error code otherwise
 */
void clog_queue_clear(clog_queue_t *queue);

/**
 * destroy the queue and release all resources
 * @param queue the queue to destroy
 */
void clog_queue_destroy(clog_queue_t **queue);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_QUEUE_H */
