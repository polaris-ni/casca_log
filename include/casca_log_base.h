/**
 * @author Polaris
 * @date  2025/10/8
 */
#ifndef CASCA_LOG_CASCA_LOG_BASE_H
#define CASCA_LOG_CASCA_LOG_BASE_H

#include "casca_log_config.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_RET_IF(cond, ret) \
    do {                       \
        if (cond) {            \
            return ret;        \
        }                      \
    } while (0)

#define CLOG_RET_IF_NULL(ptr, ret) CLOG_RET_IF((ptr) == NULL, (ret))

#define CLOG_RET_VOID_IF(cond) \
    do {                       \
        if (cond) {            \
            return;            \
        }                      \
    } while (0)

#define CLOG_RET_VOID_IF_NULL(ptr) CLOG_RET_VOID_IF((ptr) == NULL)

#define CLOG_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifdef CASCA_LOG_DEBUG
#include <assert.h>
#define CLOG_ASSERT(cond) assert(cond)
#else
#define CLOG_ASSERT(cond)
#endif

#define CLOG_UNUSED_VAR(x) (void)x
#define CLOG_IGNORE_RES(f) (void)(f)

typedef enum clog_res {
    CLOG_SUCCESS = 0, /* exec success */
    CLOG_FAIL = 1, /* exec failed */
    CLOG_NOT_SUPPORTED = 2, /* operation not supported */
    CLOG_INVALID_PARAM = 3, /* invalid param */
    CLOG_ERROR_FORMAT = 4, /* error format */
    CLOG_NO_MEMORY = 5, /* no memory, malloc failed */
    CLOG_TARGET_NOT_FOUND = 6, /* something not found */
    CLOG_OVERSIZE = 7, /* oversize */
} clog_res_e;


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_BASE_H */
