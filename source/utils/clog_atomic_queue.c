/**
 * @auther Polaris
 * @date  2025/11/11
 */
#include "clog_atomic_queue.h"
#include <stdatomic.h>
#include "clog_ebr.h"
#include "clog_error.h"
#include "clog_hooks.h"
#include "clog_secure_func.h"

#define CLOG_ATOMIC_QUEUE_RUNNING 0
#define CLOG_ATOMIC_QUEUE_FINALIZING 1

typedef struct clog_queue_node {
    atomic_uintptr_t next;
    uintptr_t data;
} clog_queue_node_t;

struct clog_atomic_queue {
    atomic_uintptr_t head;
    atomic_uintptr_t tail;
    atomic_uintptr_t state;
    atomic_uintptr_t release_num;
    clog_deallocator_f deallocator;
    clog_ebr_global_t *global;
};

struct clog_atomic_queue_handle {
    clog_atomic_queue_bias_e bias;
    clog_ebr_thread_local_t *local;
    clog_atomic_queue_t *queue;
};

static void clog_atomic_queue_node_free(void *ptr)
{
    clog_free(ptr);
}

clog_res_e clog_atomic_queue_create(clog_atomic_queue_t **queue, clog_deallocator_f deallocator)
{
    CLOG_RET_IF_NULL_X(queue, CLOG_INVALID_PARAM, "tmp is NULL");
    CLOG_RET_IF_NULL_X(deallocator, CLOG_INVALID_PARAM, "deallocator is NULL");
    clog_atomic_queue_t *tmp = clog_malloc(sizeof(clog_atomic_queue_t));
    CLOG_RET_IF_NULL_X(tmp, CLOG_NO_MEMORY, "malloc clog_atomic_queue_t failed");
    atomic_init(&tmp->head, 0);
    atomic_init(&tmp->tail, 0);
    atomic_init(&tmp->state, CLOG_ATOMIC_QUEUE_RUNNING);
    atomic_init(&tmp->release_num, 0);
    tmp->deallocator = deallocator;
    clog_queue_node_t *dummy = clog_malloc(sizeof(clog_queue_node_t));
    if (dummy == NULL) {
        CLOG_ERR_ADD("acquire dummy node failed");
        clog_free(tmp);
        return CLOG_NO_MEMORY;
    }
    atomic_init(&dummy->next, 0);
    atomic_init(&tmp->head, (uintptr_t)dummy);
    atomic_init(&tmp->tail, (uintptr_t)dummy);
    dummy->next = 0;
    tmp->global = NULL;
    const clog_res_e ret = clog_ebr_create(&tmp->global, clog_atomic_queue_node_free);
    if (ret == CLOG_SUCCESS) {
        *queue = tmp;
    } else {
        clog_free(dummy);
        clog_free(tmp);
    }
    return ret;
}

clog_atomic_queue_handle_t clog_atomic_queue_attach(clog_atomic_queue_t *queue, clog_atomic_queue_bias_e bias)
{
    CLOG_RET_IF_NULL(queue, NULL);
    clog_atomic_queue_handle_t handle = clog_malloc(sizeof(*handle));
    CLOG_RET_IF_NULL(handle, NULL);
    handle->bias = bias;
    handle->queue = queue;
    const clog_res_e ret = clog_ebr_register(queue->global, &handle->local);
    CLOG_CLEAN_RET_IF_X(ret != CLOG_SUCCESS, clog_free(handle), NULL, "register ebr failed, ret = %u", ret);
    return handle;
}

static void clog_atomic_queue_gc(clog_atomic_queue_handle_t handle, bool is_enqueue)
{
    if (atomic_load(&handle->queue->release_num) <= 128) {
        return;
    }
    if (handle->bias == CLOG_ATOMIC_QUEUE_BIASED_ANY) {
        clog_ebr_local_poll(handle->local);
        atomic_store(&handle->queue->release_num, 0);
    } else if (handle->bias == CLOG_ATOMIC_QUEUE_BIASED_ENQUEUE) {
        if (is_enqueue) {
            clog_ebr_local_poll(handle->local);
            atomic_store(&handle->queue->release_num, 0);
        }
    } else if (handle->bias == CLOG_ATOMIC_QUEUE_BIASED_DEQUEUE) {
        if (!is_enqueue) {
            clog_ebr_local_poll(handle->local);
            atomic_store(&handle->queue->release_num, 0);
        }
    } else {
        /* not perform gc on this thread */
    }
}

clog_res_e clog_atomic_queue_enqueue(clog_atomic_queue_handle_t handle, uintptr_t data)
{
    CLOG_RET_IF_NULL_X(handle, CLOG_INVALID_PARAM, "handle is NULL");
    clog_atomic_queue_gc(handle, true);
    clog_atomic_queue_t *queue = handle->queue;
    CLOG_RET_IF_X(atomic_load(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING, CLOG_ABNORMAL_STATE, "queue is not running");

    clog_queue_node_t *node = clog_malloc(sizeof(clog_queue_node_t));
    CLOG_RET_IF_NULL(node, CLOG_NO_MEMORY);
    node->data = data;
    atomic_store(&node->next, 0);

    clog_ebr_enter(handle->local);
    atomic_uintptr_t tail;
    while (1) {
        tail = atomic_load(&queue->tail);
        clog_queue_node_t *tail_node = (clog_queue_node_t *)tail;
        atomic_uintptr_t next = atomic_load(&tail_node->next);
        if (next != 0) {
            atomic_compare_exchange_strong(&queue->tail, &tail, next); /* do not care whether it is successful */
            continue;
        }
        if (tail == atomic_load(&queue->tail)) {
            CLOG_CLEAN_RET_IF_X(atomic_load(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING, clog_free(node),
                                CLOG_ABNORMAL_STATE, "queue is not running");
            uintptr_t new_tail = (uintptr_t)node;
            if (atomic_compare_exchange_strong(&tail_node->next, &next, new_tail)) {
                atomic_compare_exchange_strong(&queue->tail, &tail, new_tail);
                clog_ebr_exit(handle->local);
                return CLOG_SUCCESS;
            }
        }
    }
}

static clog_res_e clog_atomic_queue_dequeue_internal(clog_atomic_queue_t *queue, uintptr_t *data, bool need_check)
{
    uintptr_t tail;
    while (1) {
        uintptr_t head = atomic_load(&queue->head);
        tail = atomic_load(&queue->tail);
        const uintptr_t next = atomic_load(&((clog_queue_node_t *)head)->next);
        if (need_check) {
            CLOG_RET_IF_X(atomic_load(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING, CLOG_ABNORMAL_STATE,
                          "queue is not running");
        }
        if (head == tail && next == 0) {
            return CLOG_TARGET_NOT_FOUND;
        }
        if (head == tail && next != 0) {
            atomic_compare_exchange_strong(&queue->tail, &tail, next);
            continue;
        }
        if (tail != atomic_load(&queue->tail)) {
            continue;
        }

        if (next != 0) {
            if (!atomic_compare_exchange_strong(&queue->head, &head, next)) {
                continue;
            }
            clog_queue_node_t *head_node = (clog_queue_node_t *)head;
            clog_queue_node_t *next_node = (clog_queue_node_t *)next;
            *data = next_node->data;
            next_node->data = 0;
            clog_ebr_defer_release_global(queue->global, head_node);
            break;
        }
    }

    return CLOG_SUCCESS;
}

clog_res_e clog_atomic_queue_dequeue(clog_atomic_queue_handle_t handle, uintptr_t *data)
{
    CLOG_RET_IF_NULL(handle, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(data, CLOG_INVALID_PARAM);
    clog_atomic_queue_t *queue = handle->queue;
    CLOG_RET_IF_X(atomic_load(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING, CLOG_ABNORMAL_STATE, "queue is not running");
    clog_atomic_queue_gc(handle, false);
    clog_ebr_enter(handle->local);
    const clog_res_e ret = clog_atomic_queue_dequeue_internal(queue, data, true);
    clog_ebr_exit(handle->local);
    if (ret == CLOG_SUCCESS) {
        atomic_fetch_and(&handle->queue->release_num, 1);
    }
    return ret;
}

bool clog_atomic_queue_is_empty(clog_atomic_queue_handle_t handle)
{
    CLOG_RET_IF_NULL_X(handle, true, "queue is NULL");
    const atomic_uintptr_t head = atomic_load(&handle->queue->head);
    const atomic_uintptr_t tail = atomic_load(&handle->queue->tail);
    const atomic_uintptr_t next = atomic_load(&((clog_queue_node_t *)head)->next);
    return head == tail && next == 0;
}

void clog_atomic_queue_detach(clog_atomic_queue_handle_t handle)
{
    CLOG_RET_VOID_IF_NULL(handle);
    clog_ebr_unregister(handle->local);
    clog_free(handle);
}

clog_res_e clog_atomic_queue_destroy(clog_atomic_queue_t *queue)
{
    CLOG_RET_IF_NULL_X(queue, CLOG_INVALID_PARAM, "queue is NULL");
    atomic_store(&queue->state, CLOG_ATOMIC_QUEUE_FINALIZING);
    /* clear all nodes */
    uintptr_t tmp = 0;
    clog_res_e ret = clog_atomic_queue_dequeue_internal(queue, &tmp, false);
    queue->deallocator((void *)tmp);
    while (ret == CLOG_SUCCESS) {
        ret = clog_atomic_queue_dequeue_internal(queue, &tmp, false);
        queue->deallocator((void *)tmp);
    }
    /* clear dummy node */
    clog_queue_node_t *dummy = (clog_queue_node_t *)atomic_load(&queue->head);
    if (dummy != NULL) {
        clog_ebr_defer_release_global(queue->global, dummy);
    }
    /* destroy ebr */
    clog_ebr_poll(queue->global);
    CLOG_IGNORE_RES(clog_ebr_destroy(queue->global));
    clog_free(queue);
    return CLOG_SUCCESS;
}
