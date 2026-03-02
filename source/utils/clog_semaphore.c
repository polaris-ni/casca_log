/**
 * @author Polaris
 * @date  2026/2/8
 */

#include "clog_semaphore.h"
#include <stdlib.h>
#include "clog_hooks.h"
#include "clog_platform.h"

#ifdef CLOG_PLATFORM_WINDOWS
#include <stdint.h>
#include <windows.h>

struct clog_sem {
    HANDLE handle;
};

clog_sem_t *clog_sem_create(unsigned int value)
{
    clog_sem_t *sem = clog_malloc(sizeof(clog_sem_t));
    if (!sem) return NULL;

    sem->handle = CreateSemaphoreA(NULL, (LONG)value, INT32_MAX, NULL);
    if (!sem->handle) {
        clog_free(sem);
        return NULL;
    }
    return sem;
}

clog_res_e clog_sem_destroy(clog_sem_t *sem)
{
    CLOG_RET_IF_NULL(sem, CLOG_INVALID_PARAM);
    if (sem->handle && !CloseHandle(sem->handle)) {
        clog_free(sem);
        return CLOG_FAIL;
    }
    clog_free(sem);
    return CLOG_SUCCESS;
}

clog_res_e clog_sem_wait(clog_sem_t *sem, int32_t timeout)
{
    CLOG_RET_IF_NULL(sem, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(sem->handle, CLOG_INVALID_PARAM);
    const DWORD wait_time = timeout < CLOG_SEM_TIMEOUT_IMMEDIATELY ? INFINITE : (DWORD)timeout;
    const DWORD result = WaitForSingleObject(sem->handle, wait_time);

    if (result == WAIT_OBJECT_0) {
        return CLOG_SUCCESS;
    }
    if (result == WAIT_TIMEOUT) {
        return CLOG_TIMEOUT;
    }
    return CLOG_FAIL;
}

clog_res_e clog_sem_post(clog_sem_t *sem)
{
    CLOG_RET_IF_NULL(sem, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(sem->handle, CLOG_INVALID_PARAM);
    return ReleaseSemaphore(sem->handle, 1, NULL) ? CLOG_SUCCESS : CLOG_FAIL;
}

#else
#include <errno.h>
#include <semaphore.h>
#include <time.h>

struct clog_sem {
    sem_t sem;
    int is_valid;
};

clog_sem_t *clog_sem_create(unsigned int value)
{
    clog_sem_t *sem = (clog_sem_t *)clog_malloc(sizeof(clog_sem_t));
    CLOG_RET_IF_NULL(sem, NULL);

    if (sem_init(&sem->sem, 0, value) != 0) {
        clog_free(sem);
        return NULL;
    }
    sem->is_valid = 1;
    return sem;
}

clog_res_e clog_sem_destroy(clog_sem_t *sem)
{
    CLOG_RET_IF_NULL(sem, CLOG_INVALID_PARAM);
    CLOG_RET_IF(!sem->is_valid, CLOG_ABNORMAL_STATE);
    const int ret = sem_destroy(&sem->sem);
    sem->is_valid = 0;
    clog_free(sem);
    return (ret == 0) ? CLOG_SUCCESS : CLOG_FAIL;
}

clog_res_e clog_sem_wait(clog_sem_t *sem, int32_t timeout)
{
    CLOG_RET_IF_NULL(sem, CLOG_INVALID_PARAM);
    CLOG_RET_IF(!sem->is_valid, CLOG_ABNORMAL_STATE);
    if (timeout == CLOG_SEM_TIMEOUT_IMMEDIATELY) {
        if (sem_trywait(&sem->sem) == 0) {
            return CLOG_SUCCESS;
        }
        return (errno == EAGAIN) ? CLOG_TIMEOUT : CLOG_FAIL;
    }

    if (timeout == CLOG_SEM_TIMEOUT_NEVER) {
        return (sem_wait(&sem->sem) == 0) ? CLOG_SUCCESS : CLOG_FAIL;
    }

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return CLOG_FAIL;
    }

    ts.tv_sec += timeout / 1000;
    ts.tv_nsec += (timeout % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }

    const int ret = sem_timedwait(&sem->sem, &ts);
    if (ret == 0) return CLOG_SUCCESS;
    return (errno == ETIMEDOUT) ? CLOG_TIMEOUT : CLOG_FAIL;
}

clog_res_e clog_sem_post(clog_sem_t *sem)
{
    CLOG_RET_IF_NULL(sem, CLOG_INVALID_PARAM);
    CLOG_RET_IF(!sem->is_valid, CLOG_ABNORMAL_STATE);
    return (sem_post(&sem->sem) == 0) ? CLOG_SUCCESS : CLOG_FAIL;
}
#endif
