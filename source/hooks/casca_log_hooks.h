/**
 * @auther Polaris
 * @date  2025/9/3
 */
#ifndef CASCA_LOG_CASCA_LOG_HOOKS_H
#define CASCA_LOG_CASCA_LOG_HOOKS_H

#include <stddef.h>
#include "casca_log_config.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef void* (*clog_allocator_f)(size_t size);
typedef void (*clog_deallocator_f)(void* ptr);

#ifdef CASCA_LOG_HOOKS
void* clog_malloc(size_t size);
void clog_free(void* ptr);
void clog_register_memory_manager(clog_allocator_f allocator, clog_deallocator_f deallocator);
#else
#define clog_malloc malloc
#define clog_free free
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_HOOKS_H */
