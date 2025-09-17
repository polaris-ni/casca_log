/**
 * @author Polaris
 * @date  2025/9/9
 */
#ifndef CASCA_LOG_CLOG_MEM_POOL_H
#define CASCA_LOG_CLOG_MEM_POOL_H
#include <stddef.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_MP_PRE_ALLOCATED_NONE 0 /* the memory will not be pre allocated */
#define CLOG_MP_PRE_ALLOCATED_LESS 1 /* 8B/16B/32B heap will be pre allocated */
#define CLOG_MP_PRE_ALLOCATED_NORMAL 2 /* 8B/16B/32B/64B/128B heap will be pre allocated */
#define CLOG_MP_PRE_ALLOCATED_FULL 3 /* 8B/16B/32B/64B/128B/256B/512B heap will be pre allocated */

/**
 * init mem pool
 * @param level pre allocated memory num
 * @see CLOG_MP_PRE_ALLOCATED_NONE
 * @see CLOG_MP_PRE_ALLOCATED_LESS
 * @see CLOG_MP_PRE_ALLOCATED_NORMAL
 * @see CLOG_MP_PRE_ALLOCATED_FULL
 */
void clog_mp_init(size_t level);

/**
 * allocate memory from mem pool
 * if memory size <= 512B, it will be allocated from mem pool, otherwise it will be allocated from system by
 * #clog_malloc
 * @param size memory size, must be greater than 0
 * @return memory pointer, NULL if memory allocation failed or size is 0
 */
void* clog_mp_allocate(size_t size);

/**
 * release memory to mem pool if allocated from mem pool
 * free memory if allocated from system by #clog_free
 * @param ptr memory pointer, nonnull
 */
void clog_mp_release(void* ptr);

/**
 * TODO: get allocated memory size
 * @return allocated memory size
 */
size_t clog_mp_get_allocated_size(void);

/**
 * clean mem pool, release all memory, and it should be called only when thread exit and all memory are released
 * it is better to check it with #clog_mp_get_allocated_size first
 */
void clog_mp_finalize(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_MEM_POOL_H */
