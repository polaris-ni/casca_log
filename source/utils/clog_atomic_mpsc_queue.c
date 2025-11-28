/**
 * @auther polaris
 * @date  2025/11/28
 */
#include "clog_atomic_mpsc_queue.h"
#include <stdatomic.h>
#include <stdbool.h>
#include "casca_log_config.h"
#include "clog_error.h"
#include "clog_hooks.h"

typedef struct clog_atomic_mpsc_queue_node {
    atomic_uintptr_t next;
    uintptr_t data;
} clog_atomic_mpsc_queue_node_t;

typedef struct clog_atomic_mpsc_queue {
    _Alignas(CASCA_LOG_CACHE_LINE_SIZE) atomic_uintptr_t head;
    _Alignas(CASCA_LOG_CACHE_LINE_SIZE) atomic_uintptr_t tail;
} clog_atomic_mpsc_queue_t;

clog_atomic_mpsc_queue_t* clog_atomic_mpsc_queue_create(void)
{
    clog_atomic_mpsc_queue_t* queue = (clog_atomic_mpsc_queue_t*)clog_malloc(sizeof(clog_atomic_mpsc_queue_t));
    CLOG_RET_IF_NULL_X(queue, NULL, "malloc clog_atomic_mpsc_queue_t failed");
    clog_atomic_mpsc_queue_node_t* node = clog_malloc(sizeof(clog_atomic_mpsc_queue_node_t));
    CLOG_CLEAN_RET_IF_NULL_X(node, clog_free(queue), NULL, "malloc dummy node failed");
    node->data = 0;
    atomic_init(&node->next, 0);
    atomic_init(&queue->head, (uintptr_t)node);
    atomic_init(&queue->tail, (uintptr_t)node);
    return queue;
}

clog_res_e clog_atomic_mpsc_queue_in(clog_atomic_mpsc_queue_t* queue, uintptr_t data)
{
    CLOG_RET_IF_NULL(queue, CLOG_INVALID_PARAM);
    clog_atomic_mpsc_queue_node_t* node = clog_malloc(sizeof(clog_atomic_mpsc_queue_node_t));
    CLOG_RET_IF_NULL_X(node, CLOG_NO_MEMORY, "malloc clog_atomic_mpsc_queue_node_t failed");
    node->data = 0;
    atomic_init(&node->next, 0);

    while (true) {
        uintptr_t tail = atomic_load(&queue->tail);
        clog_atomic_mpsc_queue_node_t* tail_node = (clog_atomic_mpsc_queue_node_t*)tail;
        uintptr_t next = atomic_load(&tail_node->next);
        if (next != 0) {
            atomic_compare_exchange_strong(&queue->tail, &tail, next);
            continue;
        }
        if (tail != atomic_load(&queue->tail)) {
            continue;
        }
        const uintptr_t new_tail = (uintptr_t)node;
        if (atomic_compare_exchange_strong(&tail_node->next, &next, new_tail)) {
            atomic_compare_exchange_strong(&queue->tail, &tail, new_tail);
            return CLOG_SUCCESS;
        }
    }
}

clog_res_e clog_atomic_mpsc_queue_out(clog_atomic_mpsc_queue_t* queue, uintptr_t* data)
{
    CLOG_RET_IF_NULL(queue, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(data, CLOG_INVALID_PARAM);
    while (true) {
        uintptr_t head = atomic_load(&queue->head);
        uintptr_t tail = atomic_load(&queue->tail);
        clog_atomic_mpsc_queue_node_t* tail_node = (clog_atomic_mpsc_queue_node_t*)tail;
        const uintptr_t next = atomic_load(&tail_node->next);
        if (head == tail) {
            if (next == 0) {
                return CLOG_TARGET_NOT_FOUND;
            }
            atomic_compare_exchange_strong(&queue->tail, &tail, next);
            continue;
        }
        const uintptr_t head_next = atomic_load(&((clog_atomic_mpsc_queue_node_t*)head)->next);
        if (head_next == 0) {
            continue;
        }
        if (atomic_compare_exchange_strong(&queue->head, &head, head_next)) {
            *data = ((clog_atomic_mpsc_queue_node_t*)head_next)->data;
            clog_free((void*)head);
            return CLOG_SUCCESS;
        }
    }
}

void clog_atomic_mpsc_atomic_destroy(clog_atomic_mpsc_queue_t* queue)
{
    CLOG_RET_VOID_IF_NULL(queue);
    uintptr_t head = atomic_load(&queue->head);
    while (head != 0) {
        clog_atomic_mpsc_queue_node_t* head_node = (clog_atomic_mpsc_queue_node_t*)head;
        const uintptr_t next = atomic_load(&head_node->next);
        clog_free((void*)head);
        head = next;
    }
    clog_free(queue);
}
