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

typedef struct clog_ebr_global clog_ebr_global_t;
typedef struct clog_ebr_thread_local clog_ebr_thread_local_t;

/**
 * create a new global EBR state
 * @param global *global will be set to a newly allocated clog_ebr_global_t structure
 * @param free deallocator
 * @return #clog_res_e
 */
clog_res_e clog_ebr_create(clog_ebr_global_t** global, clog_deallocator_f free);

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

/**
 * destroy a global EBR state, must be called when all threads have performed #clog_ebr_unregister
 * @param global EBR to be destroyed
 */
void clog_ebr_destroy(clog_ebr_global_t* global);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_EBR_H */
