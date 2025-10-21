/**
 * @auther polaris
 * @date  2025/10/21
 */
#include "clog_buffer_pool.h"
#include "clog_atomic.h"
#include "clog_hooks.h"

#define CLOG_BUFFER_TYPE_POOL 1 /* block allocated from buffer pool, will be put back to buffer pool  */
#define CLOG_BUFFER_TYPE_TMP 2 /* block allocated from system, will be free  */

#define CLOG_STATE_DISABLED 0 /* buffer pool is not active */
#define CLOG_STATE_EXPANDING 1 /* expand buffer num */
#define CLOG_STATE_SHRINK 2 /* shrink buffer num */
#define CLOG_STATE_NORMAL 3 /* normal state */

typedef struct clog_buffer clog_buffer_t;

struct clog_buffer {
    clog_buffer_t* next;
    unsigned int type;
    clog_entry_t entry;
};

#define CLOG_CACHELINE_SIZE 64

typedef struct clog_buffer_pool {
    atomic_uintptr_t head;
    char pad0[CLOG_CACHELINE_SIZE - sizeof(atomic_uintptr_t)];

    atomic_int state;
    char pad1[CLOG_CACHELINE_SIZE - sizeof(atomic_int)];

    atomic_uint free_count;
    atomic_uint capacity;
    char pad2[CLOG_CACHELINE_SIZE - 2 * sizeof(atomic_uint)];

    size_t init_capacity;
    bool auto_manager;
    double threshold;
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

clog_res_e clog_buffer_pool_initialize(size_t init_capacity, bool auto_manager, double threshold)
{
    atomic_init(&g_buffer_pool.head, 0);
    atomic_init(&g_buffer_pool.state, CLOG_STATE_NORMAL);
    atomic_init(&g_buffer_pool.free_count, init_capacity);
    atomic_init(&g_buffer_pool.capacity, init_capacity);
    g_buffer_pool.init_capacity = init_capacity;
    g_buffer_pool.auto_manager = auto_manager;
    g_buffer_pool.threshold = threshold;
    clog_buffer_t* buffer = clog_buffer_pool_malloc(init_capacity, CLOG_BUFFER_TYPE_POOL);
    CLOG_CLEAN_RET_IF_NULL(buffer, clog_buffer_pool_finalize(), CLOG_NO_MEMORY);
    atomic_init(&g_buffer_pool.head, (uintptr_t)buffer);
    return CLOG_SUCCESS;
}

clog_entry_t* clog_buffer_pool_acquire(void)
{
    if (atomic_load_explicit(&g_buffer_pool.state, memory_order_acquire) != CLOG_STATE_NORMAL) {
        clog_buffer_t* buffer = clog_malloc(sizeof(clog_buffer_t));
        CLOG_RET_IF_NULL(buffer, NULL);
        buffer->type = CLOG_BUFFER_TYPE_TMP;
        buffer->next = NULL;
        return &buffer->entry;
    }

    uintptr_t old_head = atomic_load_explicit(&g_buffer_pool.head, memory_order_acquire);
    while (old_head != 0) {
        clog_buffer_t* buffer = (clog_buffer_t*)old_head;
        uintptr_t new_head = (uintptr_t)buffer->next;
        if (atomic_compare_exchange_strong(&g_buffer_pool.head, &old_head, new_head)) {
            atomic_fetch_sub_explicit(&g_buffer_pool.free_count, 1, memory_order_relaxed);
            return &buffer->entry;
        }
        old_head = atomic_load_explicit(&g_buffer_pool.head, memory_order_acquire);
    }

    if (!g_buffer_pool.auto_manager) {
        /* not support auto expand, just malloc from system */
        return &clog_buffer_pool_malloc(1, CLOG_BUFFER_TYPE_TMP)->entry;
    }

    int normal_state = CLOG_STATE_NORMAL;
    if (atomic_compare_exchange_strong(&g_buffer_pool.state, &normal_state, CLOG_STATE_EXPANDING)) {
        clog_buffer_t* buffer = clog_buffer_pool_malloc(g_buffer_pool.init_capacity, CLOG_BUFFER_TYPE_POOL);
        CLOG_RET_IF_NULL(buffer, NULL);
        clog_buffer_t* tail = buffer;
        while (tail->next != NULL) {
            tail = tail->next;
        }
        const uintptr_t new_head = (uintptr_t)buffer;
        uintptr_t current_head;
        do {
            current_head = atomic_load_explicit(&g_buffer_pool.head, memory_order_relaxed);
            tail->next = (clog_buffer_t*)current_head;
        } while (!atomic_compare_exchange_strong(&g_buffer_pool.head, &current_head, new_head));

        atomic_fetch_add(&g_buffer_pool.capacity, g_buffer_pool.init_capacity);
        atomic_fetch_add(&g_buffer_pool.free_count, g_buffer_pool.init_capacity);
        atomic_store_explicit(&g_buffer_pool.state, CLOG_STATE_NORMAL, memory_order_release);
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
        uintptr_t old_head;
        const uintptr_t new_head = (uintptr_t)buffer;
        do {
            old_head = atomic_load_explicit(&g_buffer_pool.head, memory_order_relaxed);
            buffer->next = (clog_buffer_t*)old_head;
        } while (!atomic_compare_exchange_strong(&g_buffer_pool.head, &old_head, new_head));
        atomic_fetch_add(&g_buffer_pool.free_count, 1);
        return;
    }

    uint32_t capacity;
    uint32_t new_capacity;
    uint32_t free_count;
    do {
        capacity = atomic_load(&g_buffer_pool.capacity);
        free_count = atomic_load(&g_buffer_pool.free_count);
        if ((free_count < capacity * g_buffer_pool.threshold) || (capacity == g_buffer_pool.init_capacity)) {
            return;
        }
        new_capacity = capacity - 1;
    } while (!atomic_compare_exchange_strong(&g_buffer_pool.capacity, &capacity, new_capacity));
    clog_free(buffer);
}

void clog_buffer_pool_finalize(void)
{
    int state = CLOG_STATE_NORMAL;
    while (!atomic_compare_exchange_strong(&g_buffer_pool.state, &state, CLOG_STATE_DISABLED)) {}
    uintptr_t addr = atomic_load(&g_buffer_pool.head);
    while (!atomic_compare_exchange_strong(&g_buffer_pool.head, &addr, 0)) {
        addr = atomic_load(&g_buffer_pool.head);
    }
    clog_buffer_t* buffer = (clog_buffer_t*)addr;
    while (buffer != NULL) {
        clog_buffer_t* next = buffer->next;
        clog_free(buffer);
        buffer = next;
    }
    atomic_init(&g_buffer_pool.free_count, 0);
    atomic_init(&g_buffer_pool.capacity, 0);
    g_buffer_pool.init_capacity = 0;
    g_buffer_pool.auto_manager = false;
}
