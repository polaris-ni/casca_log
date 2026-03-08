/**
 * @author Polaris
 * @date  2026/03/08
 */
#include "clog_queue.h"
#include "clog_error.h"
#include "clog_mutex.h"
#include "clog_secure_func.h"

typedef struct clog_queue_node {
    uintptr_t data;
    struct clog_queue_node *next;
} clog_queue_node_t;

struct clog_queue {
    clog_queue_node_t *head;
    clog_queue_node_t *tail;
    size_t size;
    clog_deallocator_f deallocator;
};

static clog_queue_node_t *clog_queue_node_create(uintptr_t ptr)
{
    clog_queue_node_t *node = clog_malloc(sizeof(clog_queue_node_t));
    CLOG_RET_IF_NULL_X(node, NULL, "failed to allocate queue node");
    node->data = ptr;
    node->next = NULL;
    return node;
}

static void clog_queue_node_free(const clog_queue_t *queue, clog_queue_node_t *node)
{
    CLOG_RET_VOID_IF_NULL(queue);
    CLOG_RET_VOID_IF_NULL(node);

    if (queue->deallocator != NULL && node->data != 0) {
        queue->deallocator((void *)node->data);
    }
    clog_free(node);
}

clog_res_e clog_queue_create(clog_queue_t **queue, clog_deallocator_f deallocator)
{
    CLOG_RET_IF_NULL_X(queue, CLOG_INVALID_PARAM, "queue output parameter is NULL");

    clog_queue_t *tmp = clog_malloc(sizeof(clog_queue_t));
    CLOG_RET_IF_NULL_X(tmp, CLOG_NO_MEMORY, "failed to allocate queue structure");

    tmp->head = NULL;
    tmp->tail = NULL;
    tmp->size = 0;
    tmp->deallocator = deallocator;

    *queue = tmp;
    return CLOG_SUCCESS;
}

clog_res_e clog_queue_enqueue(clog_queue_t *queue, uintptr_t ptr)
{
    CLOG_RET_IF_NULL_X(queue, CLOG_INVALID_PARAM, "queue is NULL");

    clog_queue_node_t *node = clog_queue_node_create(ptr);
    CLOG_RET_IF_NULL_X(node, CLOG_NO_MEMORY, "failed to create queue node");

    if (queue->tail == NULL) {
        queue->head = node;
        queue->tail = node;
    } else {
        queue->tail->next = node;
        queue->tail = node;
    }

    queue->size++;
    return CLOG_SUCCESS;
}

clog_res_e clog_queue_dequeue(clog_queue_t *queue, uintptr_t *data)
{
    CLOG_RET_IF_NULL_X(queue, CLOG_INVALID_PARAM, "queue is NULL");
    CLOG_RET_IF_NULL_X(data, CLOG_INVALID_PARAM, "data output parameter is NULL");
    CLOG_RET_IF_NULL_X(queue->head, CLOG_TARGET_NOT_FOUND, "queue is empty");

    clog_queue_node_t *node = queue->head;
    *data = node->data;
    node->data = 0;
    queue->head = node->next;

    if (queue->head == NULL) {
        queue->tail = NULL;
    }

    queue->size--;
    clog_free(node);
    return CLOG_SUCCESS;
}

clog_res_e clog_queue_peek(const clog_queue_t *queue, uintptr_t *data)
{
    CLOG_RET_IF_NULL_X(queue, CLOG_INVALID_PARAM, "queue is NULL");
    CLOG_RET_IF_NULL_X(data, CLOG_INVALID_PARAM, "data output parameter is NULL");
    CLOG_RET_IF_NULL_X(queue->head, CLOG_TARGET_NOT_FOUND, "queue is empty");

    *data = queue->head->data;
    return CLOG_SUCCESS;
}

bool clog_queue_is_empty(const clog_queue_t *queue)
{
    CLOG_RET_IF_NULL_X(queue, true, "queue is NULL");
    return queue->head == NULL;
}

size_t clog_queue_size(const clog_queue_t *queue)
{
    CLOG_RET_IF_NULL_X(queue, 0, "queue is NULL");
    return queue->size;
}

void clog_queue_clear(clog_queue_t *queue)
{
    CLOG_RET_VOID_IF_NULL_X(queue, "queue is NULL");
    clog_queue_node_t *current = queue->head;
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    while (current != NULL) {
        clog_queue_node_t *next = current->next;
        clog_queue_node_free(queue, current);
        current = next;
    }
}

void clog_queue_destroy(clog_queue_t **queue)
{
    CLOG_RET_VOID_IF_NULL_X(queue, "queue is NULL");
    CLOG_RET_VOID_IF_NULL_X(*queue, "*queue is NULL");
    clog_queue_clear(*queue);
    clog_free(*queue);
    *queue = NULL;
}
