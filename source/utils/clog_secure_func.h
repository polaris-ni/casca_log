/**
 * @auther Polaris
 * @date  2025/9/8
 */
#ifndef CASCA_LOG_CLOG_SECURE_FUNC_H
#define CASCA_LOG_CLOG_SECURE_FUNC_H

#include "casca_log.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

clog_res_e clog_memset(void* dest, size_t size, char padding, size_t count);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_SECURE_FUNC_H */
