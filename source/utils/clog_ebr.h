/**
 * @author Polaris
 * @date  2025/11/19
 */
#ifndef CASCA_LOG_CLOG_EBR_H
#define CASCA_LOG_CLOG_EBR_H

#include "casca_log_base.h"
#include "clog_hooks.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum clog_ebr_local_state {
    CLOG_EBR_LOCAL_STATE_UNKNOWN = 0,
    CLOG_EBR_LOCAL_STATE_ACTIVE = 1,
    CLOG_EBR_LOCAL_STATE_INACTIVE = 2,
    CLOG_EBR_LOCAL_STATE_RELEASED = 3
} clog_ebr_local_state_e;

typedef enum clog_ebr_global_state {
    CLOG_EBR_GLOBAL_STATE_UNKNOWN = 0,
    CLOG_EBR_GLOBAL_STATE_ACTIVE_NORMAL = 1,
    CLOG_EBR_GLOBAL_STATE_ACTIVE_GC = 2,
    CLOG_EBR_GLOBAL_STATE_INACTIVE = 3,
} clog_ebr_global_state_e;

typedef struct clog_ebr_global clog_ebr_global_t;
typedef struct clog_ebr_thread_local clog_ebr_thread_local_t;

/**
 * create a new global Epoch-Based Reclamation (EBR) state
 * @param global *global will be set to a newly allocated clog_ebr_global_t structure
 * @param free deallocator
 * @return #clog_res_e
 */
clog_res_e clog_ebr_create(clog_ebr_global_t **global, clog_deallocator_f free);

/**
 * get global state
 * @param global global EBR
 * @return #clog_ebr_global_state_e
 */
clog_ebr_global_state_e clog_ebr_get_global_state(const clog_ebr_global_t *global);

/**
 * get thread local state
 * @param local thread local EBR
 * @return #clog_ebr_local_state_e
 */
clog_ebr_local_state_e clog_ebr_get_local_state(const clog_ebr_thread_local_t *local);

/**
 * register thread itself to #global
 * @param global an initialized global EBR
 * @param local thread local EBR
 * @return #clog_res_e
 */
clog_res_e clog_ebr_register(clog_ebr_global_t *global, clog_ebr_thread_local_t **local);

/**
 * register thread in global, it must be called after the thread leave the critical section
 * @param local thread local EBR
 */
void clog_ebr_unregister(clog_ebr_thread_local_t *local);

/**
 * thread entry critical section
 * @param local thread local EBR
 * @return #CLOG_INVALID_PARAM if local or global is NULL
 * #CLOG_ABNORMAL_STATE if global state is not CLOG_EBR_GLOBAL_STATE_ACTIVE, you should call #clog_ebr_unregister
 * #CLOG_ALREADY_EXISTED if local state is CLOG_EBR_LOCAL_STATE_ACTIVE(NOTE: local epoch will be updated)
 * #CLOG_SUCCESS otherwise
 */
clog_res_e clog_ebr_enter(clog_ebr_thread_local_t *local);

/**
 * thread exit critical section
 * @param local thread local EBR
 */
void clog_ebr_exit(clog_ebr_thread_local_t *local);

/**
 * deferred memory release
 * @param global global EBR
 * @param ptr memory to be released
 */
void clog_ebr_defer_release_global(clog_ebr_global_t *global, void *ptr);

/**
 * deferred memory release
 * @param local thread local EBR
 * @param ptr memory to be released
 */
void clog_ebr_defer_release_local(const clog_ebr_thread_local_t *local, void *ptr);

/**
 * release memory by checking the status of all registered threads
 * memory that has passed the grace period will be released
 * @param global global EBR
 */
void clog_ebr_poll(clog_ebr_global_t *global);

/**
 * release memory by checking the status of all registered threads by the thread itself
 * memory that has passed the grace period will be released
 * @param local thread local EBR
 */
void clog_ebr_local_poll(clog_ebr_thread_local_t *local);

/**
 * destroy a global EBR state, must be called when all threads have performed #clog_ebr_unregister.
 * If there are threads that have not been unregistered, the actual destroy operation will be delayed
 * until all thread cancellation operations are completed and CLOG_NOT_COMPLETED will be returned
 * @param global EBR to be destroyed
 * @return #CLOG_NOT_COMPLETED if there exists an unregistered thread
 * #CLOG_ALREADY_EXISTED if clog_ebr_destroy has been called
 * #CLOG_SUCCESS otherwise
 */
clog_res_e clog_ebr_destroy(clog_ebr_global_t *global);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_EBR_H */
