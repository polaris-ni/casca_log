/**
 * @auther Polaris
 * @date  2025/11/19
 */
#include "clog_ebr.h"
#include <stdatomic.h>
#include <stdbool.h>
#include "casca_log_base.h"
#include "clog_hooks.h"
#include "clog_thread.h"

#define CLOG_EBR_NUM_EPOCHS 3

typedef struct clog_ebr_memory_node {
    void* ptr;
    atomic_uintptr_t next;
} clog_ebr_memory_node_t;

typedef struct clog_ebr_retired_list {
    atomic_uintptr_t head;
} clog_ebr_retired_list_t;

struct clog_ebr_global {
    atomic_int state;
    char pad0[CLOG_CACHE_LINE_SIZE - sizeof(atomic_int)];
    atomic_uint_fast64_t epoch;
    char pad1[CLOG_CACHE_LINE_SIZE - sizeof(atomic_uint_fast64_t)];
    atomic_uintptr_t threads;
    char pad2[CLOG_CACHE_LINE_SIZE - sizeof(atomic_uintptr_t)];
    clog_ebr_retired_list_t memory[CLOG_EBR_NUM_EPOCHS];
    char pad3[CLOG_CACHE_LINE_SIZE - sizeof(clog_ebr_retired_list_t) * CLOG_EBR_NUM_EPOCHS];
    clog_deallocator_f free;
    char pad4[CLOG_CACHE_LINE_SIZE - sizeof(clog_deallocator_f)];
};

struct clog_ebr_thread_local {
    clog_ebr_global_t* global;
    char pad0[CLOG_CACHE_LINE_SIZE - sizeof(clog_ebr_global_t*)];
    atomic_uint_fast64_t epoch;
    char pad1[CLOG_CACHE_LINE_SIZE - sizeof(atomic_uint_fast64_t)];
    atomic_int state;
    char pad2[CLOG_CACHE_LINE_SIZE - sizeof(atomic_int)];
    atomic_uintptr_t next;
    char pad3[CLOG_CACHE_LINE_SIZE - sizeof(atomic_uintptr_t)];
};

clog_res_e clog_ebr_create(clog_ebr_global_t** global, clog_deallocator_f free)
{
    CLOG_RET_IF_NULL(global, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(free, CLOG_INVALID_PARAM);
    clog_ebr_global_t* tmp = (clog_ebr_global_t*)clog_malloc(sizeof(clog_ebr_global_t));
    CLOG_RET_IF_NULL(tmp, CLOG_NO_MEMORY);
    atomic_init(&tmp->state, CLOG_EBR_GLOBAL_STATE_ACTIVE_NORMAL);
    atomic_init(&tmp->epoch, 0);
    const size_t list_size = CLOG_ARRAY_SIZE(tmp->memory);
    for (size_t i = 0; i < list_size; ++i) {
        atomic_init(&tmp->memory[i].head, 0);
    }
    atomic_init(&tmp->threads, 0);
    tmp->free = free;
    *global = tmp;
    return CLOG_SUCCESS;
}

clog_ebr_global_state_e clog_ebr_get_global_state(const clog_ebr_global_t* global)
{
    CLOG_RET_IF_NULL(global, CLOG_EBR_GLOBAL_STATE_UNKNOWN);
    const int state = atomic_load(&global->state);
    if (state >= CLOG_EBR_GLOBAL_STATE_ACTIVE_NORMAL && state <= CLOG_EBR_GLOBAL_STATE_INACTIVE) {
        return (clog_ebr_global_state_e)state;
    }
    return CLOG_EBR_GLOBAL_STATE_UNKNOWN;
}

clog_ebr_local_state_e clog_ebr_get_local_state(const clog_ebr_thread_local_t* local)
{
    CLOG_RET_IF_NULL(local, CLOG_EBR_LOCAL_STATE_UNKNOWN);
    const int state = atomic_load(&local->state);
    if (state >= CLOG_EBR_LOCAL_STATE_ACTIVE && state <= CLOG_EBR_LOCAL_STATE_RELEASED) {
        return (clog_ebr_local_state_e)state;
    }
    return CLOG_EBR_LOCAL_STATE_UNKNOWN;
}

clog_res_e clog_ebr_register(clog_ebr_global_t* global, clog_ebr_thread_local_t** local)
{
    CLOG_RET_IF_NULL(global, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(local, CLOG_INVALID_PARAM);

    clog_ebr_thread_local_t* tmp = (clog_ebr_thread_local_t*)clog_malloc(sizeof(clog_ebr_thread_local_t));
    CLOG_RET_IF_NULL(tmp, CLOG_NO_MEMORY);
    tmp->global = global;
    const uint_fast64_t current_epoch = atomic_load(&global->epoch);
    atomic_init(&tmp->epoch, current_epoch);
    atomic_init(&tmp->state, CLOG_EBR_LOCAL_STATE_INACTIVE);

    const uintptr_t new_head = (uintptr_t)tmp;
    uintptr_t old_head;
    do {
        old_head = atomic_load(&global->threads);
        atomic_store(&tmp->next, old_head);
        const clog_ebr_global_state_e state = clog_ebr_get_global_state(global);
        if (state != CLOG_EBR_GLOBAL_STATE_ACTIVE_NORMAL && state != CLOG_EBR_GLOBAL_STATE_ACTIVE_GC) {
            clog_free(tmp);
            return CLOG_ABNORMAL_STATE;
        }
    } while (!atomic_compare_exchange_strong(&global->threads, &old_head, new_head));

    *local = tmp;
    return CLOG_SUCCESS;
}

static clog_res_e clog_ebr_try_destroy(clog_ebr_global_t* global)
{
    const clog_ebr_global_state_e state = clog_ebr_get_global_state(global);
    if (state == CLOG_EBR_GLOBAL_STATE_ACTIVE_NORMAL || state == CLOG_EBR_GLOBAL_STATE_ACTIVE_GC) {
        return CLOG_ABNORMAL_STATE;
    }
    clog_ebr_thread_local_t* head = (clog_ebr_thread_local_t*)atomic_load(&global->threads);
    while (head != NULL) {
        if (clog_ebr_get_local_state(head) != CLOG_EBR_LOCAL_STATE_RELEASED) {
            return CLOG_NOT_COMPLETED;
        }
        head = (clog_ebr_thread_local_t*)atomic_load(&head->next);
    }

    for (size_t i = 0; i < CLOG_EBR_NUM_EPOCHS; ++i) {
        clog_ebr_memory_node_t* entry = (clog_ebr_memory_node_t*)atomic_load(&global->memory[i].head);
        atomic_store(&global->memory[i].head, 0);
        while (entry != NULL) {
            clog_ebr_memory_node_t* next = (clog_ebr_memory_node_t*)atomic_load(&entry->next);
            global->free(entry->ptr);
            clog_free(entry);
            entry = next;
        }
    }

    head = (clog_ebr_thread_local_t*)atomic_load(&global->threads);
    while (head != NULL) {
        clog_ebr_thread_local_t* next = (clog_ebr_thread_local_t*)atomic_load(&head->next);
        clog_free(head);
        head = next;
    }
    clog_free(global);
    return CLOG_SUCCESS;
}

void clog_ebr_unregister(clog_ebr_thread_local_t* local)
{
    CLOG_RET_VOID_IF_NULL(local);

    int state;
    do {
        state = atomic_load_explicit(&local->state, memory_order_acquire);
    } while (!atomic_compare_exchange_strong(&local->state, &state, CLOG_EBR_LOCAL_STATE_RELEASED));
    CLOG_IGNORE_RES(clog_ebr_try_destroy(local->global));
}

clog_res_e clog_ebr_enter(clog_ebr_thread_local_t* local)
{
    CLOG_RET_IF_NULL(local, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(local->global, CLOG_INVALID_PARAM);

    const uint_fast64_t current_epoch = atomic_load_explicit(&local->global->epoch, memory_order_acquire);
    atomic_store_explicit(&local->epoch, current_epoch, memory_order_release);
    int local_state;
    do {
        local_state = atomic_load_explicit(&local->state, memory_order_acquire);
        if (local_state == CLOG_EBR_LOCAL_STATE_ACTIVE) {
            return CLOG_ALREADY_EXISTED;
        }
        const clog_ebr_global_state_e global_state = clog_ebr_get_global_state(local->global);
        if (global_state != CLOG_EBR_GLOBAL_STATE_ACTIVE_NORMAL && global_state != CLOG_EBR_GLOBAL_STATE_ACTIVE_GC) {
            return CLOG_ABNORMAL_STATE;
        }
    } while (!atomic_compare_exchange_strong(&local->state, &local_state, CLOG_EBR_LOCAL_STATE_ACTIVE));
    return CLOG_SUCCESS;
}

void clog_ebr_exit(clog_ebr_thread_local_t* local)
{
    CLOG_RET_VOID_IF_NULL(local);

    int state;
    do {
        state = atomic_load_explicit(&local->state, memory_order_acquire);
    } while (!atomic_compare_exchange_strong(&local->state, &state, CLOG_EBR_LOCAL_STATE_INACTIVE));
}

void clog_ebr_defer_release(const clog_ebr_thread_local_t* local, void* ptr)
{
    CLOG_RET_VOID_IF_NULL(local);
    CLOG_RET_VOID_IF_NULL(local->global);
    CLOG_RET_VOID_IF_NULL(ptr);

    clog_ebr_memory_node_t* entry = (clog_ebr_memory_node_t*)clog_malloc(sizeof(clog_ebr_memory_node_t));
    CLOG_CLEAN_RET_VOID_IF_NULL(entry, local->global->free(ptr));
    entry->ptr = ptr;

    uintptr_t old_head;
    clog_ebr_retired_list_t* list;
    do {
        const uint_fast64_t current_epoch = atomic_load(&local->global->epoch);
        const uint_fast64_t index = current_epoch % CLOG_EBR_NUM_EPOCHS;
        list = &local->global->memory[index];
        old_head = atomic_load(&list->head);
        atomic_store(&entry->next, old_head);
    } while (!atomic_compare_exchange_strong(&list->head, &old_head, (uintptr_t)entry));
}

void clog_ebr_poll(clog_ebr_global_t* global)
{
    CLOG_RET_VOID_IF_NULL(global);

    int state = clog_ebr_get_global_state(global);
    CLOG_RET_VOID_IF(state == CLOG_EBR_GLOBAL_STATE_ACTIVE_GC || state == CLOG_EBR_GLOBAL_STATE_INACTIVE);
    if (!atomic_compare_exchange_strong(&global->state, &state, CLOG_EBR_GLOBAL_STATE_ACTIVE_GC)) {
        return;
    }

    uint_fast64_t current_epoch = atomic_load_explicit(&global->epoch, memory_order_acquire);
    const uint_fast64_t index = (current_epoch - 2 + CLOG_EBR_NUM_EPOCHS) % CLOG_EBR_NUM_EPOCHS;
    clog_ebr_memory_node_t* entry_head = (clog_ebr_memory_node_t*)global->memory[index].head;
    while (entry_head != NULL) {
        clog_ebr_memory_node_t* next = (clog_ebr_memory_node_t*)entry_head->next;
        global->free(entry_head->ptr);
        clog_free(entry_head);
        entry_head = next;
    }
    atomic_store(&global->memory[index].head, 0);

    while (true) {
        const uintptr_t head = atomic_load(&global->threads);
        if (head == 0) {
            atomic_store(&global->state, CLOG_EBR_GLOBAL_STATE_ACTIVE_NORMAL);
            return;
        }
        const clog_ebr_thread_local_t* local = (clog_ebr_thread_local_t*)head;
        while (local != NULL) {
            if (atomic_load_explicit(&local->state, memory_order_acquire) == CLOG_EBR_LOCAL_STATE_ACTIVE) {
                atomic_store(&global->state, CLOG_EBR_GLOBAL_STATE_ACTIVE_NORMAL);
                return;
            }
            local = (clog_ebr_thread_local_t*)atomic_load(&local->next);
        }
        if (head == atomic_load(&global->threads)) {
            break;
        }
        /* new thread registered, rescan the state of threads */
    }
    current_epoch = (current_epoch + 1) % CLOG_EBR_NUM_EPOCHS;
    atomic_store_explicit(&global->epoch, current_epoch, memory_order_release);
    atomic_store(&global->state, CLOG_EBR_GLOBAL_STATE_ACTIVE_NORMAL);
}

clog_res_e clog_ebr_destroy(clog_ebr_global_t* global)
{
    CLOG_RET_IF_NULL(global, CLOG_INVALID_PARAM);
    int state;
    while (true) {
        state = clog_ebr_get_global_state(global);
        CLOG_RET_IF(state == CLOG_EBR_GLOBAL_STATE_INACTIVE, CLOG_ALREADY_EXISTED);
        if (state == CLOG_EBR_GLOBAL_STATE_ACTIVE_NORMAL &&
            atomic_compare_exchange_strong(&global->state, &state, CLOG_EBR_GLOBAL_STATE_INACTIVE)) {
            return clog_ebr_try_destroy(global);
        }
        clog_thread_sleep(10); /* for retry */
    }
}
