/**
 * @author Polaris
 * @date 2026/3/8
 */

#ifndef CASCA_LOG_CLOG_RWLOCK_H
#define CASCA_LOG_CLOG_RWLOCK_H

#include "casca_log_base.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct clog_rwlock clog_rwlock_t;

/**
 * create rwlock
 * @return #clog_rwlock_t
 */
clog_rwlock_t *clog_rwlock_create(void);

/**
 * destroy rwlock
 * @param rwlock rwlock
 */
void clog_rwlock_destroy(clog_rwlock_t **rwlock);

/**
 * acquire read lock
 * @param rwlock #clog_rwlock_t
 * @return #clog_res_e
 */
clog_res_e clog_rwlock_rd_lock(clog_rwlock_t *rwlock);

/**
 * acquire write lock
 * @param rwlock #clog_rwlock_t
 * @return #clog_res_e
 */
clog_res_e clog_rwlock_wr_lock(clog_rwlock_t *rwlock);

/**
 * release read lock
 * @param rwlock #clog_rwlock_t
 * @return #clog_res_e
 */
void clog_rwlock_rd_unlock(clog_rwlock_t *rwlock);

/**
 * release write lock
 * @param rwlock #clog_rwlock_t
 * @return #clog_res_e
 */
void clog_rwlock_wr_unlock(clog_rwlock_t *rwlock);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_RWLOCK_H */
