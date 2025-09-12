/**
 * @auther Polaris
 * @date  2025/9/9
 */
#ifndef CASCA_LOG_CASCA_LOG_DEFINES_H
#define CASCA_LOG_CASCA_LOG_DEFINES_H

#include "casca_log_level.h"
#include <stddef.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_RET_IF(cond, ret) \
    do {                       \
        if (cond) {            \
            return ret;        \
        }                      \
    } while (0)

#define CLOG_RET_IF_NULL(ptr, ret) CLOG_RET_IF((ptr) == NULL, ret)

#define CLOG_RET_VOID_IF_NULL(ptr) CLOG_RET_IF_NULL(ptr, )

#define CLOG_ARRAY_SIZE(arr) sizeof(arr) / sizeof((arr)[0])

typedef enum clog_res {
    CLOG_SUCCESS = 0, /* exec success */
    CLOG_FAIL = 1, /* exec failed */
    CLOG_NOT_SUPPORTED = 2, /* operation not supported */
    CLOG_INVALID_PARAM = 3, /* invalid param */
} clog_res_e;

typedef struct clog_config {
    struct {

    } global;
    clog_level_e level;
} clog_config_t;

typedef struct clog_context {
    const char* process;
    char err[256];
    clog_config_t config;
} clog_context_t;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_DEFINES_H */
