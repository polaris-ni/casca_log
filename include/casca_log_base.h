/**
 * @author Polaris
 * @date  2025/10/8
 */
#ifndef CASCA_LOG_CASCA_LOG_BASE_H
#define CASCA_LOG_CASCA_LOG_BASE_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "casca_log_config.h"
#include "utils/clog_platform.h"

#ifdef CLOG_COMPILER_MSVC
#define _CRT_SECURE_NO_WARNINGS 1 /* suppress warning: warning C4996: 'x': This function or variable may be unsafe */
#endif

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

#define CLOG_RET_IF_FAILED(ret) CLOG_RET_IF((ret) != CLOG_SUCCESS, (ret))

#define CLOG_RET_VOID_IF(cond) \
    do {                       \
        if (cond) {            \
            return;            \
        }                      \
    } while (0)

#define CLOG_RET_VOID_IF_NULL(ptr) CLOG_RET_VOID_IF((ptr) == NULL)

#define CLOG_RET_VOID_IF_FAILED(ret) CLOG_RET_VOID_IF((ret) != CLOG_SUCCESS)

#define CLOG_CLEAN_RET_IF(cond, clean, ret) \
    do {                                    \
        if (cond) {                         \
            (clean);                        \
            return ret;                     \
        }                                   \
    } while (0)

#define CLOG_CLEAN_RET_IF_NULL(ptr, clean, ret) CLOG_CLEAN_RET_IF((ptr) == NULL, clean, ret)

#define CLOG_CLEAN_RET_IF_FAILED(ret, clean) CLOG_CLEAN_RET_IF((ret) != CLOG_SUCCESS, (clean), (ret))

#define CLOG_SAFE_FREE(mem)          \
    do {                             \
        if ((mem) != NULL) {         \
            clog_free((void*)(mem)); \
            (mem) = NULL;            \
        }                            \
    } while (0)

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
    CLOG_NOT_PERMITTED = 8, /* operation not permitted */
    CLOG_REQUEST_FLUSH = 9, /* request flush */
    CLOG_ALREADY_EXISTED = 10, /* something already existed */
} clog_res_e;

typedef enum clog_level {
    CLOG_LEVEL_OFF = 0, /* log is not allowed to write */
    CLOG_LEVEL_TRACE = 1, /* 1 << 0 */
    CLOG_LEVEL_DEBUG = 2, /* 1 << 1 */
    CLOG_LEVEL_INFO = 4, /* 1 << 2 */
    CLOG_LEVEL_WARN = 8, /* 1 << 3 */
    CLOG_LEVEL_ERROR = 16, /* 1 << 4 */
    CLOG_LEVEL_FETAL = 32, /* 1 << 5 */
} clog_level_e;

typedef struct clog_item {
    const char* filename;
    const char* function;
    const char* module;
    uint64_t tid;
    uint32_t line;
    clog_level_e level;
    struct {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
        uint16_t millisecond;
    };
    const char* fmt;
    va_list args;
    const char* content;
} clog_item_t;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_BASE_H */
