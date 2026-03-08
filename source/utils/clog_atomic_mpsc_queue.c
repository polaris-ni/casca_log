/**
 * @auther polaris
 * @date  2025/11/28
 */
#include "clog_atomic_mpsc_queue.h"
#include <stdatomic.h>
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

clog_atomic_mpsc_queue_t *clog_atomic_mpsc_queue_create(void)
{
    clog_atomic_mpsc_queue_t *queue = clog_malloc(sizeof(clog_atomic_mpsc_queue_t));
    CLOG_RET_IF_NULL_X(queue, NULL, "malloc clog_atomic_mpsc_queue_t failed");
    clog_atomic_mpsc_queue_node_t *node = clog_malloc(sizeof(clog_atomic_mpsc_queue_node_t));
    CLOG_CLEAN_RET_IF_NULL_X(node, clog_free(queue), NULL, "malloc dummy node failed");
    node->data = 0;
    atomic_init(&node->next, 0);
    atomic_init(&queue->head, (uintptr_t)node);
    atomic_init(&queue->tail, (uintptr_t)node);
    return queue;
}

clog_res_e clog_atomic_mpsc_queue_in(clog_atomic_mpsc_queue_t *queue, uintptr_t data)
{
    CLOG_RET_IF_NULL(queue, CLOG_INVALID_PARAM);

    clog_atomic_mpsc_queue_node_t *node = clog_malloc(sizeof(clog_atomic_mpsc_queue_node_t));
    CLOG_RET_IF_NULL_X(node, CLOG_NO_MEMORY, "malloc failed");

    node->data = data;
    atomic_init(&node->next, 0);

    const uintptr_t prev_tail = atomic_exchange_explicit(&queue->tail, (uintptr_t)node, memory_order_release);
    clog_atomic_mpsc_queue_node_t *prev_tail_node = (clog_atomic_mpsc_queue_node_t *)prev_tail;

    atomic_store_explicit(&prev_tail_node->next, (uintptr_t)node, memory_order_release);

    return CLOG_SUCCESS;
}

clog_res_e clog_atomic_mpsc_queue_out(clog_atomic_mpsc_queue_t *queue, uintptr_t *data)
{
    CLOG_RET_IF_NULL(queue, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(data, CLOG_INVALID_PARAM);

    clog_atomic_mpsc_queue_node_t *head =
        (clog_atomic_mpsc_queue_node_t *)atomic_load_explicit(&queue->head, memory_order_relaxed);

    clog_atomic_mpsc_queue_node_t *next =
        (clog_atomic_mpsc_queue_node_t *)atomic_load_explicit(&head->next, memory_order_acquire);

    if (next == NULL) {
        return CLOG_TARGET_NOT_FOUND;
    }

    *data = next->data;

    atomic_store_explicit(&queue->head, (uintptr_t)next, memory_order_release);

    atomic_thread_fence(memory_order_seq_cst);
    clog_free(head);

    return CLOG_SUCCESS;
}

void clog_atomic_mpsc_queue_destroy(clog_atomic_mpsc_queue_t *queue)
{
    CLOG_RET_VOID_IF_NULL(queue);
    uintptr_t head = atomic_load(&queue->head);
    while (head != 0) {
        clog_atomic_mpsc_queue_node_t *head_node = (clog_atomic_mpsc_queue_node_t *)head;
        const uintptr_t next = atomic_load(&head_node->next);
        clog_free((void *)head);
        head = next;
    }
    clog_free(queue);
}
