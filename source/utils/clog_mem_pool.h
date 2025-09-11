/**
 * @auther Polaris
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
 * @param level pre allocated memory num,
 */
void clog_mp_init(size_t level);

void* clog_mp_allocate(size_t size);

void clog_mp_release(void* ptr);

/**
 * clean mem pool, release all memory, and it should be called only when thread exit
 */
void clog_mp_finalize(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_MEM_POOL_H */
