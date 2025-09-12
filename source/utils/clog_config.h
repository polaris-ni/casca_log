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

clog_res_e clog_load_config(clog_context_t* ctx, const char* data);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_CONFIG_H */
