/**
 * @author Polaris
 * @date  2026/2/8
 */

#ifndef CASCA_LOG_CLOG_SEMAPHORE_H
#define CASCA_LOG_CLOG_SEMAPHORE_H

#include "casca_log_base.h"

#define CLOG_SEM_TIMEOUT_NEVER (-1)
#define CLOG_SEM_TIMEOUT_IMMEDIATELY 0
#define CLOG_SEM_TIMEOUT_MS(x) (x)

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct clog_sem clog_sem_t;

/**
 * init semaphore
 * @param value initial value
 * @return clog_sem_t
 */
clog_sem_t *clog_sem_create(unsigned int value);

/**
 * destroy semaphore
 * @param sem semaphore
 * @return
 */
clog_res_e clog_sem_destroy(clog_sem_t *sem);

/**
 * wait semaphore
 * @param sem semaphore
 * @param timeout CLOG_SEM_TIMEOUT_NEVER / CLOG_SEM_TIMEOUT_IMMEDIATELY / CLOG_SEM_TIMEOUT_MS(x)
 * @return CLOG_SUCCESS if wait success, CLOG_TIMEOUT if wait timeout, CLOG_FAIL if wait failed
 */
clog_res_e clog_sem_wait(clog_sem_t *sem, int32_t timeout);

/**
 * post semaphore
 * @param sem semaphore
 * @return CLOG_SUCCESS if post success, CLOG_FAIL if post failed
 */
clog_res_e clog_sem_post(clog_sem_t *sem);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif // CASCA_LOG_CLOG_SEMAPHORE_H
