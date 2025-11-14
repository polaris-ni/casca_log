/**
 * @author Polaris
 * @date  2025/11/14
 */
#ifndef CASCA_LOG_CLOG_THREAD_H
#define CASCA_LOG_CLOG_THREAD_H

#include "casca_log_base.h"
#include "clog_platform.h"

#ifdef CLOG_PLATFORM_WINDOWS
#include <windows.h>
#elif defined(CLOG_PLATFORM_LINUX) || defined(CLOG_PLATFORM_UNIX) || defined(CLOG_PLATFORM_MACOS)
#include <pthread.h>
#else
#error "platform not supported now"
#endif

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
typedef HANDLE clog_thread_t;
typedef DWORD clog_thread_id_t;
#else
typedef pthread_t clog_thread_t;
typedef pthread_t clog_thread_id_t;
#endif

/**
 * work function
 */
typedef void (*clog_thread_routine_f)(void* arg, size_t size);

typedef struct clog_thread_attr clog_thread_attr_t;

/**
 * init thread attribute
 * @param attr #clog_thread_attr_t
 * @return #clog_res_e
 */
clog_res_e clog_thread_attr_init(clog_thread_attr_t* attr);

/**
 * destroy thread attribute
 * @param attr #clog_thread_attr_t
 */
void clog_thread_attr_destroy(clog_thread_attr_t* attr);

/**
 * set thread stack size
 * @param attr #clog_thread_attr_t
 * @param stack_size size of stack
 * @return #clog_res_e
 */
clog_res_e clog_thread_attr_set_stack_size(clog_thread_attr_t* attr, size_t stack_size);

/**
 * get current thread id
 * @return thread id
 */
clog_thread_id_t clog_thread_self(void);

/**
 * create a thread
 * @param thread thread
 * @param attr #clog_thread_attr_t
 * @param routine work function
 * @param arg param of work function
 * @param size size of args
 * @return #clog_res_e
 */
clog_res_e clog_thread_create(clog_thread_t* thread, const clog_thread_attr_t* attr, clog_thread_routine_f routine,
                              void* arg, size_t size);

/**
 * wait thread shutdown
 * @param thread thread
 * @param ret_val return value
 * @return #clog_res_e
 */
clog_res_e clog_thread_join(clog_thread_t thread, void** ret_val);

/**
 * detach thread
 * @param thread thread
 * @return #clog_res_e
 */
clog_res_e clog_thread_detach(clog_thread_t thread);

/**
 * exit thread
 * @param ret_val return value
 */
void clog_thread_exit(void* ret_val);

/**
 * yield thread
 */
void clog_thread_yield(void);

/**
 * sleep thread
 * @param milliseconds time
 */
void clog_thread_sleep(unsigned int milliseconds);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_THREAD_H */
