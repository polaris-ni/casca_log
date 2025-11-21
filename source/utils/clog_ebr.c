/**
 * @auther Polaris
 * @date  2025/11/19
 */
#include "clog_ebr.h"
#include <stdatomic.h>
#include <stdbool.h>
#include "casca_log_base.h"
#include "clog_hooks.h"

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
    atomic_uint_fast64_t current_epoch;
    clog_ebr_retired_list_t retired_lists[CLOG_EBR_NUM_EPOCHS];
    atomic_uintptr_t registered_threads;
    clog_deallocator_f free;
};

struct clog_ebr_thread_local {
    clog_ebr_global_t* global;
    atomic_uint_fast64_t local_epoch;
    atomic_int state;
    atomic_uintptr_t next;
};

static const uint_fast64_t EBR_UNINITIALIZED_EPOCH = UINT_FAST64_MAX;

clog_res_e clog_ebr_create(clog_ebr_global_t** global, clog_deallocator_f free)
{
    CLOG_RET_IF_NULL(global, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(free, CLOG_INVALID_PARAM);
    clog_ebr_global_t* tmp = (clog_ebr_global_t*)clog_malloc(sizeof(clog_ebr_global_t));
    CLOG_RET_IF_NULL(tmp, CLOG_NO_MEMORY);
    atomic_init(&tmp->state, CLOG_EBR_GLOBAL_STATE_ACTIVE);
    atomic_init(&tmp->current_epoch, 0);
    const size_t list_size = CLOG_ARRAY_SIZE(tmp->retired_lists);
    for (size_t i = 0; i < list_size; ++i) {
        atomic_init(&tmp->retired_lists[i].head, 0);
    }
    atomic_init(&tmp->registered_threads, 0);
    tmp->free = free;
    *global = tmp;
    return CLOG_SUCCESS;
}

clog_ebr_global_state_e clog_ebr_get_global_state(const clog_ebr_global_t* global)
{
    CLOG_RET_IF_NULL(global, CLOG_EBR_GLOBAL_STATE_UNKNOWN);
    const int state = atomic_load(&global->state);
    if (state == CLOG_EBR_GLOBAL_STATE_ACTIVE || state == CLOG_EBR_GLOBAL_STATE_INACTIVE) {
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
    atomic_init(&tmp->local_epoch, EBR_UNINITIALIZED_EPOCH);
    atomic_init(&tmp->state, CLOG_EBR_LOCAL_STATE_INACTIVE);

    const uintptr_t new_head = (uintptr_t)tmp;
    uintptr_t old_head;
    do {
        old_head = atomic_load(&global->registered_threads);
        atomic_store(&tmp->next, old_head);
        const clog_ebr_global_state_e state = clog_ebr_get_global_state(global);
        CLOG_CLEAN_RET_IF(state != CLOG_EBR_GLOBAL_STATE_ACTIVE, clog_free(tmp), CLOG_ABNORMAL_STATE);
    } while (!atomic_compare_exchange_strong(&global->registered_threads, &old_head, new_head));

    *local = tmp;
    return CLOG_SUCCESS;
}

static clog_res_e clog_ebr_try_destroy(clog_ebr_global_t* global)
{
    if (clog_ebr_get_global_state(global) == CLOG_EBR_GLOBAL_STATE_ACTIVE) {
        return CLOG_ABNORMAL_STATE;
    }
    clog_ebr_thread_local_t* head = (clog_ebr_thread_local_t*)atomic_load(&global->registered_threads);
    while (head != NULL) {
        if (clog_ebr_get_local_state(head) != CLOG_EBR_LOCAL_STATE_RELEASED) {
            return CLOG_NOT_COMPLETED;
        }
        head = (clog_ebr_thread_local_t*)atomic_load(&head->next);
    }

    for (size_t i = 0; i < CLOG_EBR_NUM_EPOCHS; ++i) {
        clog_ebr_memory_node_t* entry = (clog_ebr_memory_node_t*)atomic_load(&global->retired_lists[i].head);
        atomic_store(&global->retired_lists[i].head, 0);
        while (entry != NULL) {
            clog_ebr_memory_node_t* next = (clog_ebr_memory_node_t*)atomic_load(&entry->next);
            global->free(entry->ptr);
            clog_free(entry);
            entry = next;
        }
    }

    head = (clog_ebr_thread_local_t*)atomic_load(&global->registered_threads);
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

    uint32_t state;
    do {
        state = atomic_load_explicit(&local->state, memory_order_acquire);
    } while (!atomic_compare_exchange_strong(&local->state, &state, CLOG_EBR_LOCAL_STATE_RELEASED));
    CLOG_IGNORE_RES(clog_ebr_try_destroy(local->global));
}

clog_res_e clog_ebr_enter(clog_ebr_thread_local_t* local)
{
    CLOG_RET_IF_NULL(local, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(local->global, CLOG_INVALID_PARAM);

    const uint_fast64_t current_epoch = atomic_load_explicit(&local->global->current_epoch, memory_order_acquire);
    atomic_store_explicit(&local->local_epoch, current_epoch, memory_order_release);
    uint32_t state;
    do {
        state = atomic_load_explicit(&local->state, memory_order_acquire);
        if (state == CLOG_EBR_LOCAL_STATE_ACTIVE) {
            return CLOG_ALREADY_EXISTED;
        }
        const clog_ebr_global_state_e global_state = clog_ebr_get_global_state(local->global);
        CLOG_RET_IF(global_state != CLOG_EBR_GLOBAL_STATE_ACTIVE, CLOG_ABNORMAL_STATE);
    } while (!atomic_compare_exchange_strong(&local->state, &state, CLOG_EBR_LOCAL_STATE_ACTIVE));
    return CLOG_SUCCESS;
}

void clog_ebr_exit(clog_ebr_thread_local_t* local)
{
    CLOG_RET_VOID_IF_NULL(local);

    uint32_t state;
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
        const uint_fast64_t current_epoch = atomic_load_explicit(&local->global->current_epoch, memory_order_relaxed);
        const uint_fast64_t index = current_epoch % CLOG_EBR_NUM_EPOCHS;
        list = &local->global->retired_lists[index];
        old_head = atomic_load_explicit(&list->head, memory_order_relaxed);
        atomic_store(&entry->next, old_head);
    } while (!atomic_compare_exchange_strong(&list->head, &old_head, (uintptr_t)entry));
}

void clog_ebr_poll(clog_ebr_global_t* global)
{
    CLOG_RET_VOID_IF_NULL(global);

    uint_fast64_t current_epoch = atomic_load_explicit(&global->current_epoch, memory_order_acquire);
    const uint_fast64_t index = (current_epoch + CLOG_EBR_NUM_EPOCHS) % CLOG_EBR_NUM_EPOCHS;
    clog_ebr_memory_node_t* entry_head = (clog_ebr_memory_node_t*)global->retired_lists[index].head;
    while (entry_head != NULL) {
        clog_ebr_memory_node_t* next = (clog_ebr_memory_node_t*)entry_head->next;
        global->free(entry_head->ptr);
        clog_free(entry_head);
        entry_head = next;
    }
    atomic_store(&global->retired_lists[index].head, 0);

    while (true) {
        const uintptr_t head = atomic_load(&global->registered_threads);
        if (head == 0) {
            return;
        }
        clog_ebr_thread_local_t* thread = (clog_ebr_thread_local_t*)head;
        while (thread != NULL) {
            if (atomic_load_explicit(&thread->state, memory_order_acquire) == CLOG_EBR_LOCAL_STATE_ACTIVE) {
                return;
            }
            thread = (clog_ebr_thread_local_t*)atomic_load(&thread->next);
        }
        if (head == atomic_load(&global->registered_threads)) {
            break;
        }
    }
    current_epoch = (current_epoch + 1) % CLOG_EBR_NUM_EPOCHS;
    atomic_store_explicit(&global->current_epoch, current_epoch, memory_order_release);
}

clog_res_e clog_ebr_destroy(clog_ebr_global_t* global)
{
    CLOG_RET_IF_NULL(global, CLOG_INVALID_PARAM);
    atomic_store(&global->state, CLOG_EBR_GLOBAL_STATE_INACTIVE);
    return clog_ebr_try_destroy(global);
}
