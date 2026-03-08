/**
 * @author Polaris
 * @date 2026/3/8
 */

#include "clog_rwlock.h"
#include "clog_error.h"
#include "clog_hooks.h"
#include "clog_platform.h"

#ifdef CLOG_PLATFORM_WINDOWS
#include <windows.h>

struct clog_rwlock {
    SRWLOCK lock;
};

clog_rwlock_t *clog_rwlock_create(void)
{
    clog_rwlock_t *rwlock = clog_malloc(sizeof(clog_rwlock_t));
    CLOG_RET_IF_NULL_X(rwlock, NULL, "malloc clog_rwlock_t failed");
    InitializeSRWLock(&rwlock->lock);
    return rwlock;
}

void clog_rwlock_destroy(clog_rwlock_t **rwlock)
{
    if (rwlock != NULL && *rwlock != NULL) {
        clog_free(*rwlock);
        *rwlock = NULL;
    }
}

clog_res_e clog_rwlock_rd_lock(clog_rwlock_t *rwlock)
{
    CLOG_RET_IF_NULL_X(rwlock, CLOG_INVALID_PARAM, "rwlock is null");
    AcquireSRWLockShared(&rwlock->lock);
    return CLOG_SUCCESS;
}

clog_res_e clog_rwlock_wr_lock(clog_rwlock_t *rwlock)
{
    CLOG_RET_IF_NULL_X(rwlock, CLOG_INVALID_PARAM, "rwlock is null");
    AcquireSRWLockExclusive(&rwlock->lock);
    return CLOG_SUCCESS;
}

void clog_rwlock_rd_unlock(clog_rwlock_t *rwlock)
{
    CLOG_RET_VOID_IF_NULL_X(rwlock, "rwlock is null");
    ReleaseSRWLockShared(&rwlock->lock);
}

void clog_rwlock_wr_unlock(clog_rwlock_t *rwlock)
{
    CLOG_RET_VOID_IF_NULL_X(rwlock, "rwlock is null");
    ReleaseSRWLockExclusive(&rwlock->lock);
}

#else

#include <errno.h>
#include <pthread.h>

struct clog_rwlock {
    pthread_rwlock_t lock;
};

clog_rwlock_t *clog_rwlock_create(void)
{

    clog_rwlock_t *rwlock = clog_malloc(sizeof(clog_rwlock_t));
    CLOG_RET_IF_NULL_X(rwlock, NULL, "malloc clog_rwlock_t failed");
    const int ret = pthread_rwlock_init(&rwlock->lock, NULL);
    CLOG_CLEAN_RET_IF_X(ret != 0, clog_free(rwlock), NULL, "pthread_rwlock_init failed, ret = %d", ret);
    return rwlock;
}

void clog_rwlock_destroy(clog_rwlock_t **rwlock)
{
    if (rwlock != NULL && *rwlock != NULL) {
        CLOG_IGNORE_RES(pthread_rwlock_destroy(&((*rwlock)->lock)));
        clog_free(*rwlock);
        *rwlock = NULL;
    }
}

clog_res_e clog_rwlock_rd_lock(clog_rwlock_t *rwlock)
{
    CLOG_RET_IF_NULL_X(rwlock, CLOG_INVALID_PARAM, "rwlock is null");
    const int ret = pthread_rwlock_rdlock(&rwlock->lock);
    if (ret == 0) {
        return CLOG_SUCCESS;
    }
    CLOG_ERR_ADD("pthread_rwlock_rdlock failed, ret = %d", ret);
    if (ret == EAGAIN) {
        return CLOG_OVERSIZE;
    }
    if (ret == EDEADLK) {
        return CLOG_ABNORMAL_STATE; /* deadlock */
    }
    return CLOG_FAIL;
}

clog_res_e clog_rwlock_wr_lock(clog_rwlock_t *rwlock)
{
    CLOG_RET_IF_NULL_X(rwlock, CLOG_INVALID_PARAM, "rwlock is null");
    const int ret = pthread_rwlock_wrlock(&rwlock->lock);
    if (ret == 0) {
        return CLOG_SUCCESS;
    }
    CLOG_ERR_ADD("pthread_rwlock_wrlock failed, ret = %d", ret);
    if (ret == EDEADLK) {
        return CLOG_ABNORMAL_STATE; /* deadlock */
    }
    return CLOG_FAIL;
}

void clog_rwlock_rd_unlock(clog_rwlock_t *rwlock)
{
    CLOG_RET_VOID_IF_NULL_X(rwlock, "rwlock is null");
    CLOG_IGNORE_RES(pthread_rwlock_unlock(&rwlock->lock));
}

void clog_rwlock_wr_unlock(clog_rwlock_t *rwlock)
{
    CLOG_RET_VOID_IF_NULL_X(rwlock, "rwlock is null");
    CLOG_IGNORE_RES(pthread_rwlock_unlock(&rwlock->lock));
}

#endif
