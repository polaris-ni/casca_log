/**
 * @author Polaris
 * @date  2025/9/3
 */
#ifndef CASCA_LOG_CLOG_HOOKS_H
#define CASCA_LOG_CLOG_HOOKS_H

#include <stddef.h>
#include "casca_log_config.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef void* (*clog_allocator_f)(size_t size);
typedef void (*clog_deallocator_f)(void* ptr);

#ifdef CASCA_LOG_HOOKS
/**
 * memory allocate
 * @param size size of memory
 * @return memory pointer, NULL if size is 0
 */
void* clog_malloc(size_t size);

/**
 * memory deallocate
 * @param ptr memory pointer, nonnull
 */
void clog_free(void* ptr);

/**
 * register memory hook function
 * @param allocator self-implement memory allocator, nonnull
 * @param deallocator self-implement memory deallocator, nonnull
 */
void clog_register_memory_hook_func(clog_allocator_f allocator, clog_deallocator_f deallocator);
#else
#define clog_malloc malloc
#define clog_free free
#define clog_register_memory_hook_func(allocator, deallocator)
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_HOOKS_H */
