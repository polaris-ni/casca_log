/**
 * @auther Polaris
 * @date  2025/11/19
 */
#include "clog_ebr.h"
#include <stdbool.h>
#include "casca_log_base.h"
#include "clog_hooks.h"

static const uint_fast64_t EBR_UNINITIALIZED_EPOCH = UINT_FAST64_MAX;

clog_res_e clog_ebr_create(clog_ebr_global_t** global, clog_deallocator_f free)
{
    CLOG_RET_IF_NULL(global, CLOG_INVALID_PARAM);
    clog_ebr_global_t* tmp = (clog_ebr_global_t*)clog_malloc(sizeof(clog_ebr_global_t));
    CLOG_RET_IF_NULL(tmp, CLOG_NO_MEMORY);
    tmp->free = free;
    atomic_init(&tmp->current_epoch, 0);
    const size_t list_size = CLOG_ARRAY_SIZE(tmp->retired_lists);
    for (size_t i = 0; i < list_size; ++i) {
        atomic_init(&tmp->retired_lists[i].head, 0);
    }
    atomic_init(&tmp->registered_threads, 0);
    *global = tmp;
    return CLOG_SUCCESS;
}

void clog_ebr_destroy(clog_ebr_global_t* global)
{
    CLOG_RET_VOID_IF_NULL(global);
    clog_free(global);
}

clog_res_e clog_ebr_register(clog_ebr_global_t* global, clog_ebr_thread_local_t** local)
{
    CLOG_RET_IF_NULL(global, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(local, CLOG_INVALID_PARAM);
    clog_ebr_thread_local_t* tmp = (clog_ebr_thread_local_t*)clog_malloc(sizeof(clog_ebr_thread_local_t));
    CLOG_RET_IF_NULL(tmp, CLOG_NO_MEMORY);
    tmp->global = global;
    atomic_init(&tmp->local_epoch, EBR_UNINITIALIZED_EPOCH);
    atomic_init(&tmp->state, CLOG_EBR_LOCAL_INACTIVE);

    const uintptr_t new_head = (uintptr_t)tmp;
    uintptr_t old_head;
    do {
        old_head = atomic_load(&global->registered_threads);
        atomic_store(&tmp->next, old_head);
    } while (!atomic_compare_exchange_strong(&global->registered_threads, &old_head, new_head));

    *local = tmp;
    return CLOG_SUCCESS;
}

void clog_ebr_unregister(clog_ebr_thread_local_t* local)
{
    CLOG_RET_VOID_IF_NULL(local);

    uint32_t state;
    do {
        state = atomic_load_explicit(&local->state, memory_order_acquire);
    } while (!atomic_compare_exchange_strong(&local->state, &state, CLOG_EBR_LOCAL_RELEASED));
}

void clog_ebr_enter(clog_ebr_global_t* global, clog_ebr_thread_local_t* local)
{
    CLOG_RET_VOID_IF_NULL(global);
    CLOG_RET_VOID_IF_NULL(local);

    const uint_fast64_t current_epoch = atomic_load_explicit(&global->current_epoch, memory_order_acquire);
    atomic_store_explicit(&local->local_epoch, current_epoch, memory_order_release);
    uint32_t state;
    do {
        state = atomic_load_explicit(&local->state, memory_order_acquire);
    } while (!atomic_compare_exchange_strong(&local->state, &state, CLOG_EBR_LOCAL_ACTIVE));
}

void clog_ebr_exit(clog_ebr_thread_local_t* local)
{
    CLOG_RET_VOID_IF_NULL(local);

    uint32_t state;
    do {
        state = atomic_load_explicit(&local->state, memory_order_acquire);
    } while (!atomic_compare_exchange_strong(&local->state, &state, CLOG_EBR_LOCAL_INACTIVE));
}

void clog_ebr_defer_release(clog_ebr_global_t* global, const clog_ebr_thread_local_t* local, void* ptr)
{
    CLOG_RET_VOID_IF_NULL(global);
    CLOG_RET_VOID_IF_NULL(local);
    CLOG_RET_VOID_IF_NULL(ptr);

    clog_ebr_entry_t* entry = (clog_ebr_entry_t*)clog_malloc(sizeof(clog_ebr_entry_t));
    CLOG_CLEAN_RET_VOID_IF_NULL(entry, global->free(ptr));
    entry->ptr = ptr;


    uintptr_t old_head;
    clog_ebr_retired_list_t* list;
    do {
        const uint_fast64_t current_epoch = atomic_load_explicit(&global->current_epoch, memory_order_relaxed);
        const uint_fast64_t index = current_epoch % CLOG_EBR_NUM_EPOCHS;
        list = &global->retired_lists[index];
        old_head = atomic_load_explicit(&list->head, memory_order_relaxed);
        atomic_store(&entry->next, old_head);
    } while (!atomic_compare_exchange_strong(&list->head, &old_head, (uintptr_t)entry));
}

void clog_ebr_poll(clog_ebr_global_t* global)
{
    CLOG_RET_VOID_IF_NULL(global);

    uint_fast64_t current_epoch = atomic_load_explicit(&global->current_epoch, memory_order_acquire);

    const uint_fast64_t index = (current_epoch + CLOG_EBR_NUM_EPOCHS) % CLOG_EBR_NUM_EPOCHS;
    clog_ebr_entry_t* entry_head = (clog_ebr_entry_t*)global->retired_lists[index].head;
    while (entry_head != NULL) {
        clog_ebr_entry_t* next = (clog_ebr_entry_t*)entry_head->next;
        global->free(entry_head->ptr);
        clog_free(entry_head);
        entry_head = next;
    }
    atomic_store(&global->retired_lists[index].head, 0);

    while (true) {
        uintptr_t head = atomic_load(&global->registered_threads);
        if (head == 0) {
            return;
        }
        clog_ebr_thread_local_t* thread = (clog_ebr_thread_local_t*)head;
        while (thread != NULL) {
            if (atomic_load_explicit(&thread->state, memory_order_acquire) == CLOG_EBR_LOCAL_ACTIVE) {
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
