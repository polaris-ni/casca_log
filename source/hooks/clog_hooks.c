/**
 * @author Polaris
 * @date  2025/9/3
 */
#include "clog_hooks.h"

#ifdef CASCA_LOG_HOOK_ENABLED
#include <stdlib.h>
#include "casca_log_base.h"

static clog_allocator_f g_allocator = malloc;
static clog_deallocator_f g_deallocator = free;
static clog_post_allocate_callback_f g_allocate_callback = NULL;
static clog_post_deallocate_callback_f g_deallocate_callback = NULL;

void *clog_hook_malloc(uintptr_t trace, const char *file, const char *function, int line, size_t size)
{
    void *ptr = NULL;
    if (size > 0) {
        ptr = g_allocator(size);
    }
    if (g_allocate_callback != NULL) {
        g_allocate_callback(trace, file, function, line, size, ptr);
    }
    return ptr;
}

void clog_hook_free(uintptr_t trace, const char *file, const char *function, int line, void *ptr)
{
    if (g_deallocate_callback != NULL) {
        g_deallocate_callback(trace, file, function, line, ptr);
    }
    if (ptr != NULL) {
        g_deallocator(ptr);
    }
}

void clog_register_memory_hook_func(clog_allocator_f allocator, clog_deallocator_f deallocator,
                                    clog_post_allocate_callback_f allocate_callback,
                                    clog_post_deallocate_callback_f deallocate_callback)
{
    if (allocator != NULL) {
        g_allocator = allocator;
    }
    if (deallocator != NULL) {
        g_deallocator = deallocator;
    }
    g_allocate_callback = allocate_callback;
    g_deallocate_callback = deallocate_callback;
}
#endif
