/**
 * @auther Polaris
 * @date  2025/9/3
 */
#include "clog_hooks.h"

#include <stdlib.h>

#ifdef CASCA_LOG_HOOKS
static clog_allocator_f g_allocator = malloc;
static clog_deallocator_f g_deallocator = free;

void* clog_malloc(const size_t size) { return g_allocator(size); }

void clog_free(void* ptr) { g_deallocator(ptr); }

void clog_register_memory_hook_func(clog_allocator_f allocator, clog_deallocator_f deallocator)
{
    if (allocator != NULL) {
        g_allocator = allocator;
    }
    if (deallocator != NULL) {
        g_deallocator = deallocator;
    }
}
#endif
