/**
 * @auther polaris
 * @date  2025/10/21
 */
#include "clog_buffer_pool.h"
#include "clog_atomic_types.h"
#include "clog_error.h"
#include "clog_hooks.h"

#define CLOG_BUFFER_TYPE_POOL 1 /* block allocated from buffer pool, will be put back to buffer pool  */
#define CLOG_BUFFER_TYPE_TMP 2 /* block allocated from system, will be free  */

typedef struct clog_buffer clog_buffer_t;

struct clog_buffer {
    clog_buffer_t* next;
    unsigned int type;
    char entry[0];
};

struct clog_buffer_pool {
    clog_atomic_type_t head;
    char pad0[CLOG_CACHE_LINE_SIZE - sizeof(clog_atomic_type_t)];

    clog_atomic_type_t state;
    char pad1[CLOG_CACHE_LINE_SIZE - sizeof(clog_atomic_type_t)];

    clog_atomic_type_t free_count;
    clog_atomic_type_t capacity;
    char pad2[CLOG_CACHE_LINE_SIZE - 2 * sizeof(clog_atomic_type_t)];

    size_t init_capacity;
    size_t threshold;
    bool auto_manager;
    size_t item_size;
};

static clog_buffer_t* clog_buffer_pool_malloc_single(size_t size, int type)
{
    clog_buffer_t* buffer = clog_malloc(sizeof(clog_buffer_t) + size);
    CLOG_RET_IF_NULL(buffer, NULL);
    buffer->type = type;
    buffer->next = NULL;
    return buffer;
}

static clog_buffer_t* clog_buffer_pool_malloc_pool_batch(size_t num, size_t size)
{
    size_t count = 0;
    clog_buffer_t* head = NULL;
    while (count < num) {
        clog_buffer_t* buffer = clog_buffer_pool_malloc_single(size, CLOG_BUFFER_TYPE_POOL);
        if (buffer == NULL) {
            while (head != NULL) {
                clog_buffer_t* next = head->next;
                clog_free(head);
                head = next;
            }
            return NULL;
        }
        if (head == NULL) {
            head = buffer;
            buffer->next = NULL;
        } else {
            buffer->next = head->next;
            head->next = buffer;
        }
        count++;
    }
    return head;
}

clog_res_e clog_buffer_pool_initialize(clog_buffer_pool_t** pool, size_t item_size, size_t init_capacity,
                                       bool auto_manager, uint8_t threshold)
{
    CLOG_RET_IF_NULL_X(pool, CLOG_INVALID_PARAM, "buffer pool is NULL");
    if (auto_manager && (threshold == 0) || (threshold >= 100)) {
        CLOG_ERR_ADD("buffer pool is auto-manager, but threshold is %u, it should be (0, 100)", threshold);
        return CLOG_INVALID_PARAM;
    }
    clog_buffer_pool_t* tmp = clog_malloc(sizeof(clog_buffer_pool_t));
    CLOG_RET_IF_NULL_X(tmp, CLOG_NO_MEMORY, "buffer pool is NULL");
    clog_atomic_set(&tmp->head, 0);
    clog_atomic_set(&tmp->state, CLOG_BUFFER_POOL_STATE_RUNNING);
    clog_atomic_set(&tmp->free_count, (clog_atomic_basic_t)init_capacity);
    clog_atomic_set(&tmp->capacity, (clog_atomic_basic_t)init_capacity);
    tmp->item_size = item_size;
    tmp->init_capacity = (size_t)init_capacity;
    tmp->auto_manager = auto_manager;
    tmp->threshold = threshold;
    clog_buffer_t* buffer = clog_buffer_pool_malloc_pool_batch(init_capacity, tmp->item_size);
    CLOG_CLEAN_RET_IF_NULL(buffer, clog_buffer_pool_finalize(tmp), CLOG_NO_MEMORY);
    clog_atomic_set(&tmp->head, (clog_atomic_basic_t)buffer);
    *pool = tmp;
    return CLOG_SUCCESS;
}

void* clog_buffer_pool_acquire(clog_buffer_pool_t* pool)
{
    CLOG_RET_IF_NULL_X(pool, NULL, "buffer pool is NULL");
    if (clog_buffer_pool_get_state(pool) != CLOG_BUFFER_POOL_STATE_RUNNING) {
        clog_buffer_t* buffer = clog_buffer_pool_malloc_single(pool->item_size, CLOG_BUFFER_TYPE_TMP);
        CLOG_RET_IF_NULL(buffer, NULL);
        return buffer->entry;
    }

    clog_atomic_basic_t old_head = clog_atomic_get(&pool->head);
    while (old_head != 0) {
        clog_buffer_t* buffer = (clog_buffer_t*)old_head;
        const clog_atomic_basic_t new_head = (clog_atomic_basic_t)buffer->next;
        if (clog_atomic_cas(&pool->head, &old_head, new_head)) {
            clog_atomic_fetch_sub(&pool->free_count, 1);
            buffer->next = NULL;
            return buffer->entry;
        }
        old_head = clog_atomic_get(&pool->head);
    }

    if (!pool->auto_manager) {
        /* not support auto expand, just malloc from system */
        clog_buffer_t* buffer = clog_buffer_pool_malloc_single(pool->item_size, CLOG_BUFFER_TYPE_TMP);
        CLOG_RET_IF_NULL(buffer, NULL);
        return buffer->entry;
    }

    clog_atomic_basic_t normal_state = CLOG_BUFFER_POOL_STATE_RUNNING;
    if (clog_atomic_cas(&pool->state, &normal_state, CLOG_BUFFER_POOL_STATE_EXPANDING)) {
        clog_buffer_t* buffer = clog_buffer_pool_malloc_pool_batch(pool->init_capacity, pool->item_size);
        CLOG_RET_IF_NULL(buffer, NULL);
        clog_buffer_t* tail = buffer;
        while (tail->next != NULL) {
            tail = tail->next;
        }
        const clog_atomic_basic_t new_head = (clog_atomic_basic_t)buffer;
        clog_atomic_basic_t current_head;
        do {
            current_head = clog_atomic_get(&pool->head);
            tail->next = (clog_buffer_t*)current_head;
        } while (!clog_atomic_cas(&pool->head, &current_head, new_head));

        clog_atomic_fetch_add(&pool->capacity, (clog_atomic_basic_t)pool->init_capacity);
        clog_atomic_fetch_add(&pool->free_count, (clog_atomic_basic_t)pool->init_capacity);
        clog_atomic_set(&pool->state, CLOG_BUFFER_POOL_STATE_RUNNING);
        return clog_buffer_pool_acquire(pool);
    }

    /* current state is not allowed to expand, just malloc from system */
    clog_buffer_t* buffer = clog_buffer_pool_malloc_single(pool->item_size, CLOG_BUFFER_TYPE_TMP);
    CLOG_RET_IF_NULL(buffer, NULL);
    return buffer->entry;
}

clog_buffer_pool_t* clog_buffer_pool_release(clog_buffer_pool_t* pool, void* entry)
{
    CLOG_RET_IF_NULL_X(entry, NULL, "entry is NULL");
    clog_buffer_t* buffer = (clog_buffer_t*)((uintptr_t)entry - offsetof(clog_buffer_t, entry));
    if (buffer->type == CLOG_BUFFER_TYPE_TMP) {
        clog_free(buffer);
        return pool;
    }
    CLOG_RET_IF_NULL_X(pool, NULL, "buffer pool is NULL");
    if (clog_buffer_pool_get_state(pool) == CLOG_BUFFER_POOL_STATE_FINALIZING) {
        clog_free(buffer);
        clog_atomic_fetch_sub(&pool->capacity, 1);
        if (clog_buffer_pool_get_current_capacity(pool) == 0) {
            clog_atomic_set(&pool->state, CLOG_BUFFER_POOL_STATE_DISABLED);
            clog_free(pool);
            return NULL;
        }
        return pool;
    }

    if (!pool->auto_manager) {
        clog_atomic_basic_t old_head;
        const clog_atomic_basic_t new_head = (clog_atomic_basic_t)buffer;
        do {
            old_head = clog_atomic_get(&pool->head);
            buffer->next = (clog_buffer_t*)old_head;
        } while (!clog_atomic_cas(&pool->head, &old_head, new_head));
        clog_atomic_fetch_add(&pool->free_count, 1);
        return pool;
    }

    clog_atomic_basic_t capacity;
    clog_atomic_basic_t new_capacity;
    do {
        capacity = clog_atomic_get(&pool->capacity);
        const clog_atomic_basic_t free_count = clog_atomic_get(&pool->free_count);
        const clog_atomic_basic_t rate = 100 - free_count * 100 / capacity;
        if ((rate > pool->threshold) || (capacity == pool->init_capacity)) {
            clog_atomic_basic_t old_head;
            const clog_atomic_basic_t new_head = (clog_atomic_basic_t)buffer;
            do {
                old_head = clog_atomic_get(&pool->head);
                buffer->next = (clog_buffer_t*)old_head;
            } while (!clog_atomic_cas(&pool->head, &old_head, new_head));
            clog_atomic_fetch_add(&pool->free_count, 1);
            return pool;
        }
        new_capacity = capacity - 1;
    } while (!clog_atomic_cas(&pool->capacity, &capacity, new_capacity));
    clog_free(buffer);
    return pool;
}

clog_res_e clog_buffer_pool_finalize(clog_buffer_pool_t* pool)
{
    CLOG_RET_IF_NULL(pool, CLOG_INVALID_PARAM);
    clog_atomic_basic_t state = CLOG_BUFFER_POOL_STATE_RUNNING;
    while (!clog_atomic_cas(&pool->state, &state, CLOG_BUFFER_POOL_STATE_FINALIZING)) {}
    clog_atomic_basic_t addr = clog_atomic_get(&pool->head);
    while (!clog_atomic_cas(&pool->head, &addr, 0)) {
        addr = clog_atomic_get(&pool->head);
    }
    clog_buffer_t* buffer = (clog_buffer_t*)addr;
    while (buffer != NULL) {
        clog_buffer_t* next = buffer->next;
        clog_free(buffer);
        buffer = next;
        clog_atomic_fetch_sub(&pool->free_count, 1);
        clog_atomic_fetch_sub(&pool->capacity, 1);
    }
    pool->init_capacity = 0;
    pool->auto_manager = false;
    pool->item_size = 0;
    if (clog_buffer_pool_get_current_capacity(pool) == 0) {
        clog_atomic_set(&pool->state, CLOG_BUFFER_POOL_STATE_DISABLED);
        clog_free(pool);
        return CLOG_SUCCESS;
    }
    return CLOG_NOT_COMPLETED;
}

int32_t clog_buffer_pool_get_state(const clog_buffer_pool_t* pool)
{
    CLOG_RET_IF_NULL_X(pool, CLOG_BUFFER_POOL_STATE_DISABLED, "buffer pool is NULL");
    const clog_atomic_basic_t tmp = clog_atomic_get(&pool->state);
    if ((tmp >= CLOG_BUFFER_POOL_STATE_DISABLED) && (tmp <= CLOG_BUFFER_POOL_STATE_FINALIZING)) {
        return (int32_t)tmp;
    }
    return CLOG_BUFFER_POOL_STATE_DISABLED;
}

size_t clog_buffer_pool_get_current_capacity(const clog_buffer_pool_t* pool)
{
    CLOG_RET_IF_NULL(pool, 0);
    const clog_atomic_basic_t tmp = clog_atomic_get(&pool->capacity);
    if (tmp <= 0) {
        return 0;
    }
    return tmp;
}

bool clog_buffer_pool_is_auto_manager(const clog_buffer_pool_t* pool)
{
    CLOG_RET_IF_NULL(pool, false);
    return pool->auto_manager;
}
