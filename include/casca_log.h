/**
 * @auther Polaris
 * @date  2025/9/2
 */
#ifndef CASCA_LOG_CASCA_LOG_H
#define CASCA_LOG_CASCA_LOG_H

#include "casca_log_config.h"
#include "casca_log_defines.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifdef CASCA_LOG_DEBUG
/* suppress warning: Possibly unused #include directive */
#endif

clog_context_t* clog_create(const char* process);

void clog_destroy(clog_context_t* context);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_H */
