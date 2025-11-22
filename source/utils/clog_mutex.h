/**
 * @author Polaris
 * @date  2025/11/22
 */
#ifndef CASCA_LOG_CLOG_MUTEX_H
#define CASCA_LOG_CLOG_MUTEX_H

#include "casca_log_base.h"
#include "clog_platform.h"
#ifdef CLOG_PLATFORM_WINDOWS
#include <windows.h>
#elif defined(CLOG_PLATFORM_LINUX) || defined(CLOG_PLATFORM_MACOS)
#include <pthread.h>
#else
#error "Platform Not Support Mutex"
#endif

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifdef CLOG_PLATFORM_WINDOWS
typedef CRITICAL_SECTION clog_mutex_t;
#else
typedef pthread_mutex_t clog_mutex_t;
#endif

/**
 * init mutex
 * @param mutex mutex
 * @return #CLOG_SUCCESS if mutex initialized successfully
 * #CLOG_INVALID_PARAM if the mutex is NULL
 * #CLOG_ABNORMAL_STATE if the system lacked the necessary resources (other than memory) to initialize another mutex
 * #CLOG_NO_MEMORY if insufficient memory exists to initialize the mutex
 * #CLOG_NOT_PERMITTED if the caller does not have the privilege to perform the operation
 * #CLOG_ERROR_FORMAT(RESERVED) if The attributes object referenced by attr has the robust mutex attribute set without
 *      the process-shared attribute being set
 * #CLOG_FAIL otherwise
 */
clog_res_e clog_mutex_init(clog_mutex_t* mutex);

/**
 * lock mutex
 * @param mutex mutex
 * @return #CLOG_SUCCESS if mutex is locked successfully
 * #CLOG_INVALID_PARAM if the mutex is NULL
 * #CLOG_OVERSIZE if the mutex could not be acquired because the maximum number of recursive locks has been exceeded
 * #CLOG_NOT_PERMITTED if the mutex was created with the protocol attribute having the value PTHREAD_PRIO_PROTECT and
 *      the calling thread's priority is higher than the mutex's current priority ceiling
 * #CLOG_ABNORMAL_STATE if the state protected by the mutex is not recoverable
 * #CLOG_TARGET_NOT_FOUND if the mutex is a robust mutex and the process containing the previous owning thread
 *      terminated while holding the mutex lock. The mutex lock shall be acquired by the calling thread, and
 *      it is up to the new owner to make the state consistent
 * #CLOG_ALREADY_EXISTED if the mutex type is PTHREAD_MUTEX_ERRORCHECK and the current thread already owns the mutex
 * #CLOG_FAIL otherwise
 */
clog_res_e clog_mutex_lock(clog_mutex_t* mutex);

/**
 * try to lock mutex
 * @param mutex mutex
 * @return #CLOG_SUCCESS if mutex is locked successfully
 * #CLOG_INVALID_PARAM if the mutex is NULL
 * #CLOG_OVERSIZE if the mutex could not be acquired because the maximum number of recursive locks has been exceeded
 * #CLOG_NOT_PERMITTED if the mutex was created with the protocol attribute having the value PTHREAD_PRIO_PROTECT and
 *      the calling thread's priority is higher than the mutex's current priority ceiling
 * #CLOG_ABNORMAL_STATE if the state protected by the mutex is not recoverable
 * #CLOG_TARGET_NOT_FOUND if the mutex is a robust mutex and the process containing the previous owning thread
 *      terminated while holding the mutex lock. The mutex lock shall be acquired by the calling thread, and
 *      it is up to the new owner to make the state consistent
 * #CLOG_BUSY if the mutex could not be acquired because it was already locked
 * #CLOG_FAIL otherwise
 */
clog_res_e clog_mutex_trylock(clog_mutex_t* mutex);

/**
 * unlock mutex
 * @param mutex mutex
 * @return #CLOG_SUCCESS if mutex is locked successfully
 * #CLOG_INVALID_PARAM if the mutex is NULL or the mutex does not refer to an initialized mutex object
 * #CLOG_OVERSIZE if the mutex could not be acquired because the maximum number of recursive locks has been exceeded
 * #CLOG_NOT_PERMITTED if the current thread does not own the mutex
 * #CLOG_FAIL otherwise
 */
clog_res_e clog_mutex_unlock(clog_mutex_t* mutex);

/**
 * destroy mutex
 * @param mutex mutex
 * @return #CLOG_SUCCESS if mutex is locked successfully
 * #CLOG_INVALID_PARAM if the mutex is NULL
 * #CLOG_BUSY if the mutex is currently locked
 * #CLOG_FAIL otherwise
 */
clog_res_e clog_mutex_destroy(clog_mutex_t* mutex);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_MUTEX_H */
