/**
 * @auther Polaris
 * @date  2025/11/22
 */
#include "clog_mutex.h"
#ifndef CLOG_PLATFORM_WINDOWS
#include <errno.h>
#endif

#ifdef CLOG_PLATFORM_WINDOWS

clog_res_e clog_mutex_init(clog_mutex_t* mutex)
{
    CLOG_RET_IF_NULL(mutex, CLOG_INVALID_PARAM);
    InitializeCriticalSection(mutex);
    return CLOG_SUCCESS;
}

clog_res_e clog_mutex_lock(clog_mutex_t* mutex)
{
    CLOG_RET_IF_NULL(mutex, CLOG_INVALID_PARAM);
    EnterCriticalSection(mutex);
    return CLOG_SUCCESS;
}

clog_res_e clog_mutex_trylock(clog_mutex_t* mutex)
{
    CLOG_RET_IF_NULL(mutex, CLOG_INVALID_PARAM);
    if (TryEnterCriticalSection(mutex)) {
        return CLOG_SUCCESS;
    }
    return CLOG_BUSY;
}

clog_res_e clog_mutex_unlock(clog_mutex_t* mutex)
{
    CLOG_RET_IF_NULL(mutex, CLOG_INVALID_PARAM);
    LeaveCriticalSection(mutex);
    return CLOG_SUCCESS;
}

clog_res_e clog_mutex_destroy(clog_mutex_t* mutex)
{
    CLOG_RET_IF_NULL(mutex, CLOG_INVALID_PARAM);
    DeleteCriticalSection(mutex);
    return CLOG_SUCCESS;
}

#elif defined(CLOG_PLATFORM_LINUX) || defined(CLOG_PLATFORM_MACOS)

clog_res_e clog_mutex_init(clog_mutex_t* mutex)
{
    CLOG_RET_IF_NULL(mutex, CLOG_INVALID_PARAM);
    const int result = pthread_mutex_init(mutex, NULL);
    CLOG_RET_IF(result == 0, CLOG_SUCCESS);
    switch (errno) {
        case EAGAIN:
            return CLOG_ABNORMAL_STATE;
        case ENOMEM:
            return CLOG_NO_MEMORY;
        case EPERM:
            return CLOG_NOT_PERMITTED;
        case EINVAL:
            return CLOG_ERROR_FORMAT;
        default:
            return CLOG_FAIL;
    }
}

clog_res_e clog_mutex_lock(clog_mutex_t* mutex)
{
    CLOG_RET_IF_NULL(mutex, CLOG_INVALID_PARAM);
    const int result = pthread_mutex_lock(mutex);
    CLOG_RET_IF(result == 0, CLOG_SUCCESS);
    switch (errno) {
        case EAGAIN:
            return CLOG_OVERSIZE;
        case EINVAL:
            return CLOG_NOT_PERMITTED;
        case ENOTRECOVERABLE:
            return CLOG_ABNORMAL_STATE;
        case EOWNERDEAD:
            return CLOG_TARGET_NOT_FOUND;
        case EDEADLK:
            return CLOG_ALREADY_EXISTED;
        default:
            return CLOG_FAIL;
    }
}

clog_res_e clog_mutex_trylock(clog_mutex_t* mutex)
{
    CLOG_RET_IF_NULL(mutex, CLOG_INVALID_PARAM);
    const int result = pthread_mutex_trylock(mutex);
    CLOG_RET_IF(result == 0, CLOG_SUCCESS);
    switch (errno) {
        case EAGAIN:
            return CLOG_OVERSIZE;
        case EINVAL:
            return CLOG_NOT_PERMITTED;
        case ENOTRECOVERABLE:
            return CLOG_ABNORMAL_STATE;
        case EOWNERDEAD:
            return CLOG_TARGET_NOT_FOUND;
        case EBUSY:
            return CLOG_BUSY;
        default:
            return CLOG_FAIL;
    }
}

clog_res_e clog_mutex_unlock(clog_mutex_t* mutex)
{
    CLOG_RET_IF_NULL(mutex, CLOG_INVALID_PARAM);
    const int result = pthread_mutex_unlock(mutex);
    CLOG_RET_IF(result == 0, CLOG_SUCCESS);
    switch (errno) {
        case EINVAL:
            return CLOG_INVALID_PARAM;
        case EAGAIN:
            return CLOG_OVERSIZE;
        case EPERM:
            return CLOG_NOT_PERMITTED;
        default:
            return CLOG_FAIL;
    }
}

clog_res_e clog_mutex_destroy(clog_mutex_t* mutex)
{
    CLOG_RET_IF_NULL(mutex, CLOG_INVALID_PARAM);
    const int result = pthread_mutex_destroy(mutex);
    CLOG_RET_IF(result == 0, CLOG_SUCCESS);
    if (errno == EBUSY) {
        return CLOG_BUSY;
    }
    return CLOG_FAIL;
}

#endif
