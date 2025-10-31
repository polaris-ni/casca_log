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

/**
 * init buffer pool
 * if auto_manager is true, buffer pool will expand and shrink automatically
 *  - expand: when buffer pool is full, buffer pool will expand in steps based on the initial capacity
 *  - shrink: when buffer pool's occupancy rate is below the threshold, buffer pool will shrink one buffer by one buffer
 * if auto_manager is false, buffer will malloc from system when buffer pool is full
 * @param init_capacity initial capacity
 * @param auto_manager auto expanding and shrinking
 * @param threshold auto shrinking threshold, 0 ~ 100
 * @return #clog_res_e
 */
clog_res_e clog_buffer_pool_initialize(size_t init_capacity, bool auto_manager, uint8_t threshold);

/**
 * acquire buffer from pool
 * if current state is not normal, buffer will be malloc from system
 * @return buffer
 */
clog_entry_t* clog_buffer_pool_acquire(void);

/**
 * release buffer to pool
 * when buffer pool's occupancy rate is below the threshold and current capacity is larger than the initial capacity,
 * this buffer will be shrunk and released to system
 * @return buffer
 */
void clog_buffer_pool_release(clog_entry_t* entry);

/**
 * shutdown buffer pool
 */
void clog_buffer_pool_finalize(void);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_BUFFER_POOL_H */
