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
    clog_entry_t entry;
};

typedef struct clog_buffer_pool {
    clog_atomic_type_t head;
    char pad0[CLOG_CACHE_LINE_SIZE - sizeof(clog_atomic_type_t)];

    clog_atomic_type_t state;
    char pad1[CLOG_CACHE_LINE_SIZE - sizeof(clog_atomic_type_t)];

    clog_atomic_type_t free_count;
    clog_atomic_type_t capacity;
    char pad2[CLOG_CACHE_LINE_SIZE - 2 * sizeof(clog_atomic_type_t)];

    clog_atomic_basic_t init_capacity;
    clog_atomic_basic_t threshold;
    bool auto_manager;
} clog_buffer_pool_t;

static clog_buffer_pool_t g_buffer_pool;

static clog_buffer_t* clog_buffer_pool_malloc(size_t num, int type)
{
    size_t count = 0;
    clog_buffer_t* head = NULL;
    while (count < num) {
        clog_buffer_t* buffer = clog_malloc(sizeof(clog_buffer_t));
        if (buffer == NULL) {
            while (head != NULL) {
                clog_buffer_t* next = head->next;
                clog_free(head);
                head = next;
            }
            return NULL;
        }
        buffer->type = type;
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

clog_res_e clog_buffer_pool_initialize(size_t init_capacity, bool auto_manager, uint8_t threshold)
{
    if (auto_manager && (threshold == 0) || (threshold >= 100)) {
        CLOG_ERR_ADD("buffer pool is auto-manager, but threshold is %u, it should be (0, 100)", threshold);
        return CLOG_INVALID_PARAM;
    }
    clog_atomic_set(&g_buffer_pool.head, 0);
    clog_atomic_set(&g_buffer_pool.state, CLOG_BUFFER_POOL_STATE_RUNNING);
    clog_atomic_set(&g_buffer_pool.free_count, (clog_atomic_basic_t)init_capacity);
    clog_atomic_set(&g_buffer_pool.capacity, (clog_atomic_basic_t)init_capacity);
    g_buffer_pool.init_capacity = (clog_atomic_basic_t)init_capacity;
    g_buffer_pool.auto_manager = auto_manager;
    g_buffer_pool.threshold = threshold;
    clog_buffer_t* buffer = clog_buffer_pool_malloc(init_capacity, CLOG_BUFFER_TYPE_POOL);
    CLOG_CLEAN_RET_IF_NULL(buffer, clog_buffer_pool_finalize(), CLOG_NO_MEMORY);
    clog_atomic_set(&g_buffer_pool.head, (clog_atomic_basic_t)buffer);
    return CLOG_SUCCESS;
}

clog_entry_t* clog_buffer_pool_acquire(void)
{
    if (clog_atomic_get(&g_buffer_pool.state) != CLOG_BUFFER_POOL_STATE_RUNNING) {
        clog_buffer_t* buffer = clog_malloc(sizeof(clog_buffer_t));
        CLOG_RET_IF_NULL(buffer, NULL);
        buffer->type = CLOG_BUFFER_TYPE_TMP;
        buffer->next = NULL;
        return &buffer->entry;
    }

    clog_atomic_basic_t old_head = clog_atomic_get(&g_buffer_pool.head);
    while (old_head != 0) {
        clog_buffer_t* buffer = (clog_buffer_t*)old_head;
        clog_atomic_basic_t new_head = (clog_atomic_basic_t)buffer->next;
        if (clog_atomic_cas(&g_buffer_pool.head, &old_head, new_head)) {
            clog_atomic_fetch_sub(&g_buffer_pool.free_count, 1);
            return &buffer->entry;
        }
        old_head = clog_atomic_get(&g_buffer_pool.head);
    }

    if (!g_buffer_pool.auto_manager) {
        /* not support auto expand, just malloc from system */
        return &clog_buffer_pool_malloc(1, CLOG_BUFFER_TYPE_TMP)->entry;
    }

    clog_atomic_basic_t normal_state = CLOG_BUFFER_POOL_STATE_RUNNING;
    if (clog_atomic_cas(&g_buffer_pool.state, &normal_state, CLOG_BUFFER_POOL_STATE_EXPANDING)) {
        clog_buffer_t* buffer = clog_buffer_pool_malloc(g_buffer_pool.init_capacity, CLOG_BUFFER_TYPE_POOL);
        CLOG_RET_IF_NULL(buffer, NULL);
        clog_buffer_t* tail = buffer;
        while (tail->next != NULL) {
            tail = tail->next;
        }
        const clog_atomic_basic_t new_head = (clog_atomic_basic_t)buffer;
        clog_atomic_basic_t current_head;
        do {
            current_head = clog_atomic_get(&g_buffer_pool.head);
            tail->next = (clog_buffer_t*)current_head;
        } while (!clog_atomic_cas(&g_buffer_pool.head, &current_head, new_head));

        clog_atomic_fetch_add(&g_buffer_pool.capacity, g_buffer_pool.init_capacity);
        clog_atomic_fetch_add(&g_buffer_pool.free_count, g_buffer_pool.init_capacity);
        clog_atomic_set(&g_buffer_pool.state, CLOG_BUFFER_POOL_STATE_RUNNING);
        return clog_buffer_pool_acquire();
    }

    /* current state is allowed to expand, just malloc from system */
    return &clog_buffer_pool_malloc(1, CLOG_BUFFER_TYPE_TMP)->entry;
}

void clog_buffer_pool_release(clog_entry_t* entry)
{
    CLOG_RET_VOID_IF_NULL(entry);
    clog_buffer_t* buffer = (clog_buffer_t*)((uintptr_t)entry - offsetof(clog_buffer_t, entry));
    if (buffer->type == CLOG_BUFFER_TYPE_TMP) {
        clog_free(buffer);
        return;
    }

    if (!g_buffer_pool.auto_manager) {
        clog_atomic_basic_t old_head;
        const clog_atomic_basic_t new_head = (clog_atomic_basic_t)buffer;
        do {
            old_head = clog_atomic_get(&g_buffer_pool.head);
            buffer->next = (clog_buffer_t*)old_head;
        } while (!clog_atomic_cas(&g_buffer_pool.head, &old_head, new_head));
        clog_atomic_fetch_add(&g_buffer_pool.free_count, 1);
        return;
    }

    clog_atomic_basic_t capacity;
    clog_atomic_basic_t new_capacity;
    do {
        capacity = clog_atomic_get(&g_buffer_pool.capacity);
        const clog_atomic_basic_t free_count = clog_atomic_get(&g_buffer_pool.free_count);
        const clog_atomic_basic_t rate = 100 - free_count * 100 / capacity;
        if ((rate > g_buffer_pool.threshold) || (capacity == g_buffer_pool.init_capacity)) {
            clog_atomic_basic_t old_head;
            const clog_atomic_basic_t new_head = (clog_atomic_basic_t)buffer;
            do {
                old_head = clog_atomic_get(&g_buffer_pool.head);
                buffer->next = (clog_buffer_t*)old_head;
            } while (!clog_atomic_cas(&g_buffer_pool.head, &old_head, new_head));
            clog_atomic_fetch_add(&g_buffer_pool.free_count, 1);
            return;
        }
        new_capacity = capacity - 1;
    } while (!clog_atomic_cas(&g_buffer_pool.capacity, &capacity, new_capacity));
    clog_free(buffer);
}

void clog_buffer_pool_finalize(void)
{
    clog_atomic_basic_t state = CLOG_BUFFER_POOL_STATE_RUNNING;
    while (!clog_atomic_cas(&g_buffer_pool.state, &state, CLOG_BUFFER_POOL_STATE_DISABLED)) {}
    clog_atomic_basic_t addr = clog_atomic_get(&g_buffer_pool.head);
    while (!clog_atomic_cas(&g_buffer_pool.head, &addr, 0)) {
        addr = clog_atomic_get(&g_buffer_pool.head);
    }
    clog_buffer_t* buffer = (clog_buffer_t*)addr;
    while (buffer != NULL) {
        clog_buffer_t* next = buffer->next;
        clog_free(buffer);
        buffer = next;
    }
    clog_atomic_set(&g_buffer_pool.free_count, 0);
    clog_atomic_set(&g_buffer_pool.capacity, 0);
    g_buffer_pool.init_capacity = 0;
    g_buffer_pool.auto_manager = false;
}

int32_t clog_buffer_pool_get_state(void)
{
    const clog_atomic_basic_t tmp = clog_atomic_get(&g_buffer_pool.state);
    if ((tmp < CLOG_BUFFER_POOL_STATE_DISABLED) || (tmp > CLOG_BUFFER_POOL_STATE_EXPANDING)) {
        return CLOG_BUFFER_POOL_STATE_DISABLED;
    }
    return (int32_t)tmp;
}

/**
 * get current buffer pool capacity
 * @return
 */
size_t clog_buffer_pool_get_current_capacity(void)
{
    const clog_atomic_basic_t tmp = clog_atomic_get(&g_buffer_pool.capacity);
    if (tmp <= 0) {
        return 0;
    }
    return tmp;
}

/**
 * check whether buffer pool is auto manager
 * @return true if auto manager, false otherwise
 */
bool clog_buffer_pool_is_auto_manager(void)
{
    return g_buffer_pool.auto_manager;
}
