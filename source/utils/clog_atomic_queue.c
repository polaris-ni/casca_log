/**
 * @auther Polaris
 * @date  2025/11/11
 */
#include "clog_atomic_queue.h"
#include "clog_atomic_types.h"
#include "clog_buffer_pool.h"
#include "clog_ebr.h"
#include "clog_error.h"
#include "clog_hooks.h"
#include "clog_secure_func.h"

#define CLOG_ATOMIC_QUEUE_RUNNING 0
#define CLOG_ATOMIC_QUEUE_FINALIZING 1

typedef struct clog_queue_node {
    clog_buffer_pool_t* pool;
    clog_atomic_type_t next;
    size_t len;
    char data[0];
} clog_queue_node_t;

struct clog_atomic_queue {
    clog_atomic_type_t head;
    clog_atomic_type_t tail;
    clog_atomic_type_t state;
    clog_atomic_type_t release_num;
    size_t item_size;
    clog_buffer_pool_t* pool;
    clog_ebr_global_t* global;
};

struct clog_atomic_queue_handle {
    clog_atomic_queue_bias_e bias;
    clog_ebr_thread_local_t* local;
    clog_atomic_queue_t* queue;
};

static void clog_atomic_queue_node_free(void* ptr)
{
    CLOG_RET_VOID_IF_NULL(ptr);
    clog_queue_node_t* node = ptr;
    CLOG_IGNORE_RES(clog_buffer_pool_release(node->pool, node));
}

clog_res_e clog_atomic_queue_create(clog_atomic_queue_t** queue, size_t item_size, uint8_t threshold)
{
    CLOG_RET_IF_NULL_X(queue, CLOG_INVALID_PARAM, "tmp is NULL");
    clog_atomic_queue_t* tmp = clog_malloc(sizeof(clog_atomic_queue_t));
    CLOG_RET_IF_NULL_X(tmp, CLOG_NO_MEMORY, "malloc clog_atomic_queue_t failed");
    clog_atomic_set(&tmp->head, 0);
    clog_atomic_set(&tmp->tail, 0);
    clog_atomic_set(&tmp->state, CLOG_ATOMIC_QUEUE_RUNNING);
    clog_atomic_set(&tmp->release_num, 0);
    tmp->item_size = item_size;
    const size_t element_size = sizeof(clog_queue_node_t) + item_size;
    clog_res_e ret = clog_buffer_pool_initialize(&tmp->pool, element_size, 1, true, threshold);
    if (ret != CLOG_SUCCESS) {
        CLOG_ERR_ADD("clog_buffer_pool_initialize failed, ret = %u", ret);
        goto CLEAN_QUEUE;
    }
    clog_queue_node_t* dummy = clog_buffer_pool_acquire(tmp->pool);
    if (dummy == NULL) {
        CLOG_ERR_ADD("acquire dummy node failed");
        ret = CLOG_NO_MEMORY;
        goto CLEAN_BUFFER_POOL;
    }
    dummy->pool = tmp->pool;
    clog_atomic_set(&dummy->next, 0);
    dummy->len = 0;
    clog_atomic_set(&tmp->head, (clog_atomic_basic_t)dummy);
    clog_atomic_set(&tmp->tail, (clog_atomic_basic_t)dummy);

    tmp->global = NULL;
    ret = clog_ebr_create(&tmp->global, clog_atomic_queue_node_free);
    if (ret == CLOG_SUCCESS) {
        *queue = tmp;
        return CLOG_SUCCESS;
    }

CLEAN_BUFFER_POOL:
    CLOG_IGNORE_RES(clog_buffer_pool_release(tmp->pool, (clog_queue_node_t*)clog_atomic_get(&tmp->head)));
    CLOG_IGNORE_RES(clog_buffer_pool_finalize(tmp->pool));

CLEAN_QUEUE:
    clog_free(tmp);
    return ret;
}

clog_atomic_queue_handle_t clog_atomic_queue_attach(clog_atomic_queue_t* queue, clog_atomic_queue_bias_e bias)
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
    if (clog_atomic_get(&handle->queue->release_num) <= clog_buffer_pool_get_current_capacity(handle->queue->pool)) {
        return;
    }
    if (handle->bias == CLOG_ATOMIC_QUEUE_BIASED_ANY) {
        clog_ebr_local_poll(handle->local);
        clog_atomic_set(&handle->queue->release_num, 0);
    } else if (handle->bias == CLOG_ATOMIC_QUEUE_BIASED_ENQUEUE) {
        if (is_enqueue) {
            clog_ebr_local_poll(handle->local);
            clog_atomic_set(&handle->queue->release_num, 0);
        }
    } else if (handle->bias == CLOG_ATOMIC_QUEUE_BIASED_DEQUEUE) {
        if (!is_enqueue) {
            clog_ebr_local_poll(handle->local);
            clog_atomic_set(&handle->queue->release_num, 0);
        }
    } else {
        /* not perform gc on this thread */
    }
}

clog_res_e clog_atomic_queue_enqueue(clog_atomic_queue_handle_t handle, const void* data, size_t size)
{
    CLOG_RET_IF_NULL_X(handle, CLOG_INVALID_PARAM, "handle is NULL");
    CLOG_RET_IF_NULL_X(data, CLOG_INVALID_PARAM, "data is NULL");
    clog_atomic_queue_gc(handle, true);
    clog_atomic_queue_t* queue = handle->queue;
    CLOG_RET_IF_X(clog_atomic_get(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING, CLOG_ABNORMAL_STATE,
                  "queue is not running");

    clog_queue_node_t* node = clog_buffer_pool_acquire(queue->pool);
    CLOG_RET_IF_NULL(node, CLOG_NO_MEMORY);
    node->pool = queue->pool;
    const clog_res_e ret = clog_memcpy(node->data, queue->item_size, data, size);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_buffer_pool_release(queue->pool, node), "memcpy failed, ret = %u", ret);
    node->len = size;
    clog_atomic_set(&node->next, 0);

    clog_ebr_enter(handle->local);
    clog_atomic_basic_t tail;
    while (1) {
        tail = clog_atomic_get(&queue->tail);
        clog_queue_node_t* tail_node = (clog_queue_node_t*)tail;
        const clog_atomic_basic_t next = clog_atomic_get(&tail_node->next);
        if (next != 0) {
            CLOG_IGNORE_RES(clog_atomic_cas(&queue->tail, &tail, next));
            continue;
        }
        if (tail == clog_atomic_get(&queue->tail)) {
            CLOG_CLEAN_RET_IF_X(clog_atomic_get(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING,
                                clog_buffer_pool_release(queue->pool, node), CLOG_ABNORMAL_STATE,
                                "queue is not running");
            clog_atomic_basic_t expected = 0;
            if (clog_atomic_cas(&tail_node->next, &expected, (clog_atomic_basic_t)node)) {
                CLOG_IGNORE_RES(clog_atomic_cas(&queue->tail, &tail, (clog_atomic_basic_t)node));
                clog_ebr_exit(handle->local);
                return CLOG_SUCCESS;
            }
        }
    }
}

static clog_res_e clog_atomic_queue_dequeue_internal(clog_atomic_queue_t* queue, void* data, size_t size, size_t* len,
                                                     bool need_check)
{
    bool is_oversize = false;
    size_t copy_num = 0;
    clog_atomic_basic_t tail;
    while (1) {
        clog_atomic_basic_t head = clog_atomic_get(&queue->head);
        tail = clog_atomic_get(&queue->tail);
        clog_atomic_basic_t next = clog_atomic_get(&((clog_queue_node_t*)head)->next);
        if (need_check) {
            CLOG_RET_IF_X(clog_atomic_get(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING, CLOG_ABNORMAL_STATE,
                          "queue is not running");
        }
        if (head != clog_atomic_get(&queue->head)) {
            continue;
        }
        if (head == tail && next == 0) {
            return CLOG_TARGET_NOT_FOUND;
        }
        if (head == tail && next != 0) {
            CLOG_IGNORE_RES(clog_atomic_cas(&queue->tail, &tail, next));
            continue;
        }

        if (next != 0) {
            clog_queue_node_t* head_node = (clog_queue_node_t*)head;
            const clog_queue_node_t* next_node = (clog_queue_node_t*)next;
            is_oversize = next_node->len > size;
            if (data != NULL) {
                copy_num = is_oversize ? size : next_node->len;
                CLOG_IGNORE_RES(clog_memcpy(data, size, next_node->data, copy_num));
            } else {
                copy_num = next_node->len;
            }
            if (!clog_atomic_cas(&queue->head, &head, next)) {
                continue;
            }
            CLOG_IGNORE_RES(clog_buffer_pool_release(queue->pool, head_node));
            break;
        }
    }

    if (len != NULL) {
        *len = copy_num;
    }
    return is_oversize ? CLOG_OVERSIZE : CLOG_SUCCESS;
}

clog_res_e clog_atomic_queue_dequeue(clog_atomic_queue_handle_t handle, void* data, size_t size, size_t* len)
{
    CLOG_RET_IF_NULL(handle, CLOG_INVALID_PARAM);
    clog_atomic_queue_t* queue = handle->queue;
    CLOG_RET_IF_X(clog_atomic_get(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING, CLOG_ABNORMAL_STATE,
                  "queue is not running");
    clog_atomic_queue_gc(handle, false);
    clog_ebr_enter(handle->local);
    const clog_res_e ret = clog_atomic_queue_dequeue_internal(queue, data, size, len, true);
    clog_ebr_exit(handle->local);
    if (ret == CLOG_SUCCESS) {
        clog_atomic_fetch_add(&handle->queue->release_num, 1);
    }
    return ret;
}

bool clog_atomic_queue_is_empty(clog_atomic_queue_handle_t handle)
{
    CLOG_RET_IF_NULL_X(handle, true, "queue is NULL");
    const clog_atomic_basic_t head = clog_atomic_get(&handle->queue->head);
    const clog_atomic_basic_t tail = clog_atomic_get(&handle->queue->tail);
    const clog_atomic_basic_t next = clog_atomic_get(&((clog_queue_node_t*)head)->next);
    return head == tail && next == 0;
}

void clog_atomic_queue_detach(clog_atomic_queue_handle_t handle)
{
    CLOG_RET_VOID_IF_NULL(handle);
    clog_ebr_unregister(handle->local);
    clog_free(handle);
}

clog_res_e clog_atomic_queue_destroy(clog_atomic_queue_t* queue)
{
    CLOG_RET_IF_NULL_X(queue, CLOG_INVALID_PARAM, "queue is NULL");
    clog_atomic_set(&queue->state, CLOG_ATOMIC_QUEUE_FINALIZING);
    /* clear all nodes */
    clog_res_e ret = clog_atomic_queue_dequeue_internal(queue, NULL, 0, NULL, false);
    while (ret == CLOG_SUCCESS || ret == CLOG_OVERSIZE) {
        ret = clog_atomic_queue_dequeue_internal(queue, NULL, 0, NULL, false);
    }
    /* clear dummy node */
    clog_queue_node_t* dummy = (clog_queue_node_t*)clog_atomic_get(&queue->head);
    if (dummy != NULL) {
        CLOG_IGNORE_RES(clog_buffer_pool_release(queue->pool, dummy));
    }
    /* destroy ebr */
    clog_ebr_poll(queue->global);
    CLOG_IGNORE_RES(clog_ebr_destroy(queue->global));
    ret = clog_buffer_pool_finalize(queue->pool);
    CLOG_RET_IF_FAILED_X(ret, "clog_buffer_pool_finalize failed, ret = %u", ret);
    queue->pool = NULL;
    clog_free(queue);
    return CLOG_SUCCESS;
}
