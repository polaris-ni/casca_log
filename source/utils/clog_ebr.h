/**
 * @author Polaris
 * @date  2025/11/19
 */
#ifndef CASCA_LOG_BUILD_CLOG_EBR_H
#define CASCA_LOG_BUILD_CLOG_EBR_H

#include <stdatomic.h>

#include "casca_log_base.h"
#include "clog_hooks.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct clog_ebr_global clog_ebr_global_t;
typedef struct clog_ebr_thread_local clog_ebr_thread_local_t;
typedef struct clog_ebr_entry clog_ebr_entry_t;

#define CLOG_EBR_NUM_EPOCHS 3

typedef struct {
    atomic_uintptr_t head;
} clog_ebr_retired_list_t;

struct clog_ebr_global {
    atomic_uint_fast64_t current_epoch;
    clog_ebr_retired_list_t retired_lists[CLOG_EBR_NUM_EPOCHS];
    atomic_uintptr_t registered_threads;
    clog_deallocator_f free;
};

typedef enum clog_ebr_local_state {
    CLOG_EBR_LOCAL_ACTIVE = 0,
    CLOG_EBR_LOCAL_INACTIVE = 1,
    CLOG_EBR_LOCAL_RELEASED = 2
} clog_ebr_local_state_e;

struct clog_ebr_thread_local {
    const clog_ebr_global_t* global;
    atomic_uint_fast64_t local_epoch;
    atomic_uint state;
    atomic_uintptr_t next;
};

/**
 * @brief 待回收项
 * 代表一个需要延迟释放的内存块或对象。
 */
struct clog_ebr_entry {
    void* ptr;
    atomic_uintptr_t next;
};

/**
 * create a new global EBR state
 * @param global *global will be set to a newly allocated clog_ebr_global_t structure
 * @param free deallocator
 * @return #clog_res_e
 */
clog_res_e clog_ebr_create(clog_ebr_global_t** global, clog_deallocator_f free);

/**
 * destroy a global EBR state, must be called when all threads have performed #clog_ebr_unregister
 * @param global EBR to be destroyed
 */
void clog_ebr_destroy(clog_ebr_global_t* global);

/**
 * register thread itself to #global
 * @param global an initialized global EBR
 * @param local thread local EBR
 * @return #clog_res_e
 */
clog_res_e clog_ebr_register(clog_ebr_global_t* global, clog_ebr_thread_local_t** local);

/**
 * register thread in global
 * @param local thread local EBR
 */
void clog_ebr_unregister(clog_ebr_thread_local_t* local);

/**
 * thread entry critical section
 * @param global global EBR
 * @param local thread local EBR
 */
void clog_ebr_enter(clog_ebr_global_t* global, clog_ebr_thread_local_t* local);

/**
 * thread exit critical section
 * @param local thread local EBR
 */
void clog_ebr_exit(clog_ebr_thread_local_t* local);

/**
 * deferred memory release
 * @param global global EBR
 * @param local thread local EBR
 * @param ptr memory to be released
 */
void clog_ebr_defer_release(clog_ebr_global_t* global, const clog_ebr_thread_local_t* local, void* ptr);

/**
 * release memory by checking the status of all registered threads
 * memory that has passed the grace period will be released
 * @param global global EBR
 */
void clog_ebr_poll(clog_ebr_global_t* global);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_BUILD_CLOG_EBR_H */
