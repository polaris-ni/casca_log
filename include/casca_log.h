/**
 * @auther Polaris
 * @date  2025/9/2
 */
#ifndef CASCA_LOG_CASCA_LOG_H
#define CASCA_LOG_CASCA_LOG_H

#include "casca_log_config.h"
#include "casca_log_level.h"
#ifdef CASCA_LOG_HOOKS
#include "casca_log_hooks.h"
#endif

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum clog_res {
    CLOG_SUCCESS = 0, /* exec success */
    CLOG_FAIL = 1, /* exec failed */
    CLOG_NOT_SUPPORTED = 2, /* operation not supported */
} clog_res_e;

typedef struct clog_context {
    clog_level_e level;
} clog_context_t;

clog_context_t* clog_create(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_H */
