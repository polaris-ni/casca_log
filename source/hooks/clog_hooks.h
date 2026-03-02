/**
 * @author Polaris
 * @date  2025/9/3
 */
#ifndef CASCA_LOG_CLOG_HOOKS_H
#define CASCA_LOG_CLOG_HOOKS_H

#include <stddef.h>
#include <stdint.h>
#include "casca_log_config.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef void *(*clog_allocator_f)(size_t size);
typedef void (*clog_deallocator_f)(void *ptr);
typedef void (*clog_post_allocate_callback_f)(uintptr_t trace, const char *file, const char *function, int line,
                                              size_t size, void *ptr);
typedef void (*clog_post_deallocate_callback_f)(uintptr_t trace, const char *file, const char *function, int line,
                                                void *ptr);

#ifdef CASCA_LOG_HOOK_ENABLED
#define CLOG_HOOK_TRACE_ID_INVALID SIZE_MAX

/**
 * memory allocate
 * @param trace trace id, use thread id if trace id is #CLOG_HOOK_TRACE_ID_INVALID
 * @param file file name
 * @param function function name
 * @param line line number
 * @param size size of memory
 * @return memory pointer, NULL if size is 0
 */
void *clog_hook_malloc(uintptr_t trace, const char *file, const char *function, int line, size_t size);

/**
 * @param trace trace id, use thread id if trace id is #CLOG_HOOK_TRACE_ID_INVALID
 * @param file file name
 * @param function function name
 * @param line line number
 * @param ptr memory pointer, nonnull
 */
void clog_hook_free(uintptr_t trace, const char *file, const char *function, int line, void *ptr);

/**
 * register memory hook function
 * @param allocator self-implement memory allocator
 * @param deallocator self-implement memory deallocator
 * @param allocate_callback memory allocate callback function, will be called after memory allocated(even failed)
 * @param deallocate_callback memory deallocate callback function, will be called after memory deallocated
 */
void clog_register_memory_hook_func(clog_allocator_f allocator, clog_deallocator_f deallocator,
                                    clog_post_allocate_callback_f allocate_callback,
                                    clog_post_deallocate_callback_f deallocate_callback);

#define clog_malloc(size) clog_hook_malloc(CLOG_HOOK_TRACE_ID_INVALID, CLOG_FILENAME, __func__, __LINE__, (size))
#define clog_free(size) clog_hook_free(CLOG_HOOK_TRACE_ID_INVALID, CLOG_FILENAME, __func__, __LINE__, (size))

static inline void *clog_sys_malloc(size_t size)
{
    return clog_malloc(size);
}

/**
 * memory deallocate
 * @param ptr memory pointer, nonnull
 */
static inline void clog_sys_free(void *ptr)
{
    clog_free(ptr);
}

#else
#include <stdlib.h>
#define clog_malloc(size) ((size) == 0 ? NULL : malloc(size))

#define clog_free(ptr) ((ptr) != NULL ? free(ptr) : (void)0)

static inline void *clog_sys_malloc(size_t size)
{
    return clog_malloc(size);
}

/**
 * memory deallocate
 * @param ptr memory pointer, nonnull
 */
static inline void clog_sys_free(void *ptr)
{
    clog_free(ptr);
}

#define clog_register_memory_hook_func(allocator, deallocator, allocate_callback, deallocate_callback)
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_HOOKS_H */
