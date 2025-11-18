/**
 * @auther Polaris
 * @date  2025/11/11
 */
#include "clog_atomic_queue.h"
#include "clog_atomic_types.h"
#include "clog_buffer_pool.h"
#include "clog_error.h"
#include "clog_hooks.h"
#include "clog_secure_func.h"

#define CLOG_ATOMIC_QUEUE_RUNNING 0
#define CLOG_ATOMIC_QUEUE_FINALIZING 1

typedef struct clog_queue_node {
    clog_atomic_type_t next;
    size_t len;
    char data[0];
} clog_queue_node_t;

typedef struct clog_atomic_queue {
    clog_atomic_type_t head;
    clog_atomic_type_t tail;
    clog_atomic_type_t state;
    size_t item_size;
    clog_buffer_pool_t* pool;
} clog_atomic_queue_t;

clog_res_e clog_atomic_queue_create(clog_atomic_queue_t** queue, size_t item_size, uint8_t threshold)
{
    CLOG_RET_IF_NULL_X(queue, CLOG_INVALID_PARAM, "tmp is NULL");
    clog_atomic_queue_t* tmp = clog_malloc(sizeof(clog_atomic_queue_t));
    CLOG_RET_IF_NULL_X(tmp, CLOG_NO_MEMORY, "malloc clog_atomic_queue_t failed");
    tmp->head = 0;
    tmp->tail = 0;
    tmp->pool = NULL;
    tmp->item_size = item_size;
    clog_atomic_set(&tmp->state, CLOG_ATOMIC_QUEUE_RUNNING);

    const size_t element_size = sizeof(clog_queue_node_t) + item_size;
    const clog_res_e ret = clog_buffer_pool_initialize(&tmp->pool, element_size, 1, true, threshold);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_atomic_queue_destroy(tmp), "init buffer pool failed, ret = %u", ret);

    clog_queue_node_t* dummy = clog_buffer_pool_acquire(tmp->pool);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_atomic_queue_destroy(tmp), "init buffer pool failed, ret = %u", ret);
    clog_atomic_set(&dummy->next, 0);
    dummy->len = 0;
    clog_atomic_set(&tmp->head, (clog_atomic_basic_t)dummy);
    clog_atomic_set(&tmp->tail, (clog_atomic_basic_t)dummy);
    *queue = tmp;
    return CLOG_SUCCESS;
}

clog_res_e clog_atomic_queue_enqueue(clog_atomic_queue_t* queue, const void* data, size_t size)
{
    CLOG_RET_IF_NULL_X(queue, CLOG_INVALID_PARAM, "queue is NULL");
    CLOG_RET_IF_NULL_X(data, CLOG_INVALID_PARAM, "data is NULL");
    CLOG_RET_IF_X(clog_atomic_get(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING, CLOG_ABNORMAL_STATE,
                  "queue is not running");

    clog_queue_node_t* node = clog_buffer_pool_acquire(queue->pool);
    CLOG_RET_IF_NULL(node, CLOG_NO_MEMORY);

    const clog_res_e ret = clog_memcpy(node->data, queue->item_size, data, size);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_buffer_pool_release(queue->pool, node), "memcpy failed, ret = %u", ret);
    node->len = size;
    node->next = 0;

    clog_atomic_basic_t tail;

    while (1) {
        tail = clog_atomic_get(&queue->tail);
        clog_queue_node_t* tail_node = (clog_queue_node_t*)tail;
        const clog_atomic_basic_t next = clog_atomic_get(&tail_node->next);
        if (tail == clog_atomic_get(&queue->tail)) {
            if (next == 0) {
                clog_atomic_basic_t expected = 0;
                CLOG_CLEAN_RET_IF_X(clog_atomic_get(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING,
                                    clog_buffer_pool_release(queue->pool, node), CLOG_ABNORMAL_STATE,
                                    "queue is not running");
                if (clog_atomic_cas(&tail_node->next, &expected, (clog_atomic_basic_t)node)) {
                    break;
                }
            } else {
                clog_atomic_cas(&queue->tail, &tail, next);
            }
        }
    }
    clog_atomic_cas(&queue->tail, &tail, (clog_atomic_basic_t)node);
    return CLOG_SUCCESS;
}

static clog_res_e clog_atomic_queue_dequeue_internal(clog_atomic_queue_t* queue, void* data, size_t size, size_t* len,
                                                     bool need_check)
{
    bool is_oversize = false;
    size_t copy_num = 0;
    clog_atomic_basic_t tail;
    while (1) {
        const clog_atomic_basic_t head = clog_atomic_get(&queue->head);
        tail = clog_atomic_get(&queue->tail);
        clog_atomic_basic_t next = clog_atomic_get(&((clog_queue_node_t*)head)->next);
        if (need_check) {
            CLOG_RET_IF_X(clog_atomic_get(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING, CLOG_ABNORMAL_STATE,
                          "queue is not running");
        }
        if (head == clog_atomic_get(&queue->head)) {
            if (head == tail && next == 0) {
                return CLOG_TARGET_NOT_FOUND;
            }
            if (head == tail && next != 0) {
                clog_atomic_cas(&queue->tail, &tail, next);
                continue;
            }

            if (next != 0) {
                clog_queue_node_t* head_node = (clog_queue_node_t*)head;
                clog_queue_node_t* next_node = (clog_queue_node_t*)next;
                const clog_atomic_basic_t next_of_next = clog_atomic_get(&next_node->next);
                if (next_of_next == 0) {
                    if (!clog_atomic_cas(&queue->tail, &next, queue->head)) {
                        continue;
                    }
                }
                if (clog_atomic_cas(&head_node->next, &next, next_node->next)) {
                    is_oversize = next_node->len > size;
                    if (data != NULL) {
                        copy_num = is_oversize ? size : next_node->len;
                        CLOG_IGNORE_RES(clog_memcpy(data, size, next_node->data, copy_num));
                    } else {
                        copy_num = next_node->len;
                    }
                    CLOG_IGNORE_RES(clog_buffer_pool_release(queue->pool, next_node));
                    tail = clog_atomic_get(&queue->tail);
                    const clog_queue_node_t* tail_node = (clog_queue_node_t*)tail;
                    while (clog_atomic_get(&tail_node->next) != 0) {
                        tail = clog_atomic_get(&tail_node->next);
                        tail_node = (clog_queue_node_t*)tail;
                    }
                    clog_atomic_set(&queue->tail, tail);
                    break;
                }
            }
        }
    }

    if (len != NULL) {
        *len = copy_num;
    }
    return is_oversize ? CLOG_OVERSIZE : CLOG_SUCCESS;
}

clog_res_e clog_atomic_queue_dequeue(clog_atomic_queue_t* queue, void* data, size_t size, size_t* len)
{
    CLOG_RET_IF_NULL(queue, CLOG_INVALID_PARAM);
    CLOG_RET_IF_X(clog_atomic_get(&queue->state) != CLOG_ATOMIC_QUEUE_RUNNING, CLOG_ABNORMAL_STATE,
                  "queue is not running");
    return clog_atomic_queue_dequeue_internal(queue, data, size, len, true);
}

bool clog_atomic_queue_is_empty(const clog_atomic_queue_t* queue)
{
    CLOG_RET_IF_NULL_X(queue, true, "queue is NULL");
    const clog_atomic_basic_t head = clog_atomic_get(&queue->head);
    const clog_atomic_basic_t tail = clog_atomic_get(&queue->tail);
    const clog_atomic_basic_t next = clog_atomic_get(&((clog_queue_node_t*)head)->next);
    return head == tail && next == 0;
}

clog_res_e clog_atomic_queue_destroy(clog_atomic_queue_t* queue)
{
    CLOG_RET_IF_NULL_X(queue, CLOG_INVALID_PARAM, "queue is NULL");
    clog_atomic_set(&queue->state, CLOG_ATOMIC_QUEUE_FINALIZING);

    clog_res_e ret = clog_atomic_queue_dequeue_internal(queue, NULL, 0, NULL, false);
    while (ret == CLOG_SUCCESS || ret == CLOG_OVERSIZE) {
        ret = clog_atomic_queue_dequeue_internal(queue, NULL, 0, NULL, false);
    }

    clog_queue_node_t* dummy = (clog_queue_node_t*)clog_atomic_get(&queue->head);
    if (dummy != NULL) {
        CLOG_IGNORE_RES(clog_buffer_pool_release(queue->pool, dummy));
    }
    ret = clog_buffer_pool_finalize(queue->pool);
    CLOG_RET_IF_FAILED_X(ret, "clog_buffer_pool_finalize failed, ret = %u", ret);
    queue->pool = NULL;
    clog_free(queue);
    return CLOG_SUCCESS;
}
