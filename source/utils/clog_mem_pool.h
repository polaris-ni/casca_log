/**
 * @auther Polaris
 * @date  2025/9/9
 */
#ifndef CASCA_LOG_CLOG_MEM_POOL_H
#define CASCA_LOG_CLOG_MEM_POOL_H
#include <stddef.h>
#include "casca_log_config.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

void clog_mp_init(void);

void* clog_mp_allocate(size_t size);

void clog_mp_release(void* ptr);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_MEM_POOL_H */
