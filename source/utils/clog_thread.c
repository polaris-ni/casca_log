/**
 * @auther Polaris
 * @date  2025/11/14
 */
#include "clog_thread.h"

#ifdef CLOG_PLATFORM_WINDOWS
#include <process.h>
#include "clog_error.h"
#include "clog_hooks.h"
typedef struct {
    clog_thread_routine_f routine;
    void* arg;
    void* res;
} thread_wrapper_data_t;
#else
#include <errno.h>
#include <time.h>
#include <unistd.h>
#endif

struct clog_thread_attr {
    size_t stack_size;
    int detach_state;
};

clog_res_e clog_thread_attr_init(clog_thread_attr_t* attr)
{
    CLOG_RET_IF_NULL(attr, CLOG_INVALID_PARAM);
    attr->stack_size = 0;
    attr->detach_state = 0;
    return CLOG_SUCCESS;
}

void clog_thread_attr_destroy(clog_thread_attr_t* attr)
{
    CLOG_RET_VOID_IF_NULL(attr);
    attr->detach_state = 0;
    attr->stack_size = 0;
}

clog_res_e clog_thread_attr_set_stack_size(clog_thread_attr_t* attr, size_t stack_size)
{
    CLOG_RET_IF_NULL(attr, CLOG_INVALID_PARAM);
    attr->stack_size = stack_size;
    return CLOG_SUCCESS;
}

clog_thread_id_t clog_thread_self(void)
{
#ifdef CLOG_PLATFORM_WINDOWS
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

#ifdef CLOG_PLATFORM_WINDOWS
static DWORD WINAPI thread_wrapper(LPVOID param)
{
    thread_wrapper_data_t* data = param;
    CLOG_IGNORE_RES(data->routine(data->arg));
    clog_free(data);
    return CLOG_SUCCESS;
}
#endif

clog_res_e clog_thread_create(clog_thread_t* thread, const clog_thread_attr_t* attr, clog_thread_routine_f routine,
                              void* arg)
{
    CLOG_RET_IF_NULL(thread, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(routine, CLOG_INVALID_PARAM);

#ifdef CLOG_PLATFORM_WINDOWS
    thread_wrapper_data_t* wrapper_data = clog_malloc(sizeof(thread_wrapper_data_t));
    CLOG_RET_IF_NULL_X(wrapper_data, CLOG_NO_MEMORY, "malloc thread_wrapper_data_t failed");

    wrapper_data->routine = routine;
    wrapper_data->arg = arg;

    DWORD thread_id;
    *thread = CreateThread(NULL, attr ? attr->stack_size : 0, thread_wrapper, wrapper_data, 0, &thread_id);

    if (*thread == NULL) {
        clog_free(wrapper_data);
        return CLOG_FAIL;
    }

    return CLOG_SUCCESS;
#else
    pthread_attr_t pthread_attr;
    CLOG_IGNORE_RES(pthread_attr_init(&pthread_attr));

    if (attr && attr->stack_size > 0) {
        CLOG_IGNORE_RES(pthread_attr_setstacksize(&pthread_attr, attr->stack_size));
    }
    const int result = pthread_create(thread, attr ? &pthread_attr : NULL, routine, arg);
    CLOG_IGNORE_RES(pthread_attr_destroy(&pthread_attr));
    switch (result) {
        case 0:
            return CLOG_SUCCESS;
        case EAGAIN:
            return CLOG_NO_MEMORY;
        case EINVAL:
            return CLOG_INVALID_PARAM;
        default:
            return CLOG_FAIL;
    }
#endif
}

clog_res_e clog_thread_join(clog_thread_t thread, void** ret_val)
{
#ifdef CLOG_PLATFORM_WINDOWS
    if (WaitForSingleObject(thread, INFINITE) == WAIT_FAILED) {
        return CLOG_FAIL;
    }

    if (ret_val != NULL) {
        DWORD exit_code;
        if (GetExitCodeThread(thread, &exit_code)) {
            *ret_val = clog_malloc(sizeof(int));
            if (*ret_val != NULL) {
                clog_res_e* tmp = *ret_val;
                *tmp = (int)exit_code;
            }
        } else {
            return CLOG_FAIL;
        }
    }

    CloseHandle(thread);
    return CLOG_SUCCESS;
#else
    void* result = NULL;
    const int res = pthread_join(thread, &result);
    if (ret_val) {
        *ret_val = result;
    }
    switch (res) {
        case 0:
            return CLOG_SUCCESS;
        case EDEADLK:
            return CLOG_ABNORMAL_STATE;
        case EINVAL:
            return CLOG_INVALID_PARAM;
        case ESRCH:
            return CLOG_TARGET_NOT_FOUND;
        default:
            return CLOG_FAIL;
    }
#endif
}

clog_res_e clog_thread_detach(clog_thread_t thread)
{
#ifdef CLOG_PLATFORM_WINDOWS
    return CLOG_SUCCESS;
#else
    const int result = pthread_detach(thread);
    switch (result) {
        case 0:
            return CLOG_SUCCESS;
        case EINVAL:
            return CLOG_INVALID_PARAM;
        case ESRCH:
            return CLOG_TARGET_NOT_FOUND;
        default:
            return CLOG_FAIL;
    }
#endif
}

void clog_thread_exit(void* ret_val)
{
#if defined(_WIN32) || defined(_WIN64)
    const int* res = ret_val;
    DWORD code = -1;
    if (res != NULL) {
        code = *res;
    }
    ExitThread(code);
#else
    pthread_exit(ret_val);
#endif
}

void clog_thread_yield(void)
{
#if defined(_WIN32) || defined(_WIN64)
    SwitchToThread();
#else
    CLOG_IGNORE_RES(sched_yield());
#endif
}

void clog_thread_sleep(unsigned int milliseconds)
{
#if defined(_WIN32) || defined(_WIN64)
    Sleep(milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    CLOG_IGNORE_RES(nanosleep(&ts, NULL));
#endif
}
