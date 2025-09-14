/**
 * @auther Polaris
 * @date  2025/9/12
 */
#ifndef CASCA_LOG_CLOG_CONFIG_H
#define CASCA_LOG_CLOG_CONFIG_H

#include "casca_log_defines.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 *
 * @param ctx context
 * @param data config string, should contain an end char '\0'
 * @return #clog_res_e
 */
clog_res_e clog_config_load(clog_context_t* ctx, const char* data);

bool clog_config_dump(const clog_context_t* ctx, char* buf, size_t size);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_CONFIG_H */
