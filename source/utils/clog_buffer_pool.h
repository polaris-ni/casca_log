/**
 * @author polaris
 * @date  2025/10/21
 */
#ifndef CASCA_LOG_CLOG_BUFFER_POOL_H
#define CASCA_LOG_CLOG_BUFFER_POOL_H

#include <stdbool.h>
#include "casca_log_base.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_BUFFER_POOL_STATE_DISABLED 0 /* buffer pool is not active */
#define CLOG_BUFFER_POOL_STATE_RUNNING 1 /* normal state */
#define CLOG_BUFFER_POOL_STATE_EXPANDING 2 /* expand buffer num */
#define CLOG_BUFFER_POOL_STATE_FINALIZING 3 /* buffer pool finalizing */

typedef struct clog_buffer_pool clog_buffer_pool_t;

/**
 * init buffer pool
 * if auto_manager is true, buffer pool will expand and shrink automatically
 *  - expand: when buffer pool is full, buffer pool will expand in steps based on the initial capacity
 *  - shrink: when buffer pool's occupancy rate is below the threshold, buffer pool will shrink one buffer by one buffer
 * if auto_manager is false, buffer will malloc from system when buffer pool is full
 * @param pool pointer to buffer pool, if initialized successfully, *pool will be set to a valid buffer pool
 * @param item_size single item size of buffer pool
 * @param init_capacity initial capacity
 * @param auto_manager auto expanding and shrinking
 * @param threshold auto shrinking threshold, 0 ~ 100
 * @return #clog_res_e
 */
clog_res_e clog_buffer_pool_initialize(clog_buffer_pool_t** pool, size_t item_size, size_t init_capacity,
                                       bool auto_manager, uint8_t threshold);

/**
 * acquire buffer from pool
 * if current state is not normal, buffer will be malloc from system
 * @param pool buffer pool
 * @return buffer
 */
void* clog_buffer_pool_acquire(clog_buffer_pool_t* pool);

/**
 * release buffer to pool
 * when buffer pool's occupancy rate is below the threshold and current capacity is larger than the initial capacity,
 * this buffer will be shrunk and released to system
 * @param pool buffer pool
 * @param entry buffer to be released
 * @return buffer
 */
void clog_buffer_pool_release(clog_buffer_pool_t* pool, void* entry);

/**
 * shutdown buffer pool
 * @param pool buffer pool to be cleanup
 * @return CLOG_SUCCESS if all buffers are released successfully, CLOG_NOT_COMPLETED if not
 */
clog_res_e clog_buffer_pool_finalize(clog_buffer_pool_t* pool);

/**
 * get current buffer pool state
 * @param pool buffer pool
 * @return buffer pool state
 */
int32_t clog_buffer_pool_get_state(const clog_buffer_pool_t* pool);

/**
 * get current buffer pool capacity
 * @param pool buffer pool
 * @return current buffer pool capacity
 */
size_t clog_buffer_pool_get_current_capacity(const clog_buffer_pool_t* pool);

/**
 * check whether buffer pool is auto manager
 * @param pool buffer pool
 * @return true if auto manager, false otherwise
 */
bool clog_buffer_pool_is_auto_manager(const clog_buffer_pool_t* pool);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_BUFFER_POOL_H */
