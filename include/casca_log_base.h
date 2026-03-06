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
#include "clog_platform.h"

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

#define CLOG_RET_IF_FAILED(ret)             \
    do {                                    \
        clog_res_e __ret_of_ret = (ret);    \
        if (__ret_of_ret != CLOG_SUCCESS) { \
            return __ret_of_ret;            \
        }                                   \
    } while (0)

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

#define CLOG_CLEAN_RET_VOID_IF(cond, clean) \
    do {                                    \
        if (cond) {                         \
            (clean);                        \
            return;                         \
        }                                   \
    } while (0)

#define CLOG_CLEAN_RET_VOID_IF_NULL(ptr, clean) CLOG_CLEAN_RET_VOID_IF((ptr) == NULL, clean)

#define CLOG_CLEAN_RET_VOID_IF_FAILED(ret, clean) CLOG_CLEAN_RET_VOID_IF((ret) != CLOG_SUCCESS, (clean))

#define CLOG_SAFE_FREE(mem)           \
    do {                              \
        if ((mem) != NULL) {          \
            clog_free((void *)(mem)); \
            (mem) = NULL;             \
        }                             \
    } while (0)

#define CLOG_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifdef CASCA_LOG_DEBUG
#include <assert.h>
#define CLOG_ASSERT(cond) assert(cond)
#else
#define CLOG_ASSERT(cond)
#endif

#if defined(CLOG_COMPILER_CLANG) || defined(CLOG_COMPILER_GCC)
#define CLOG_PACKED_STRUCT(name, stat) typedef struct name stat __attribute__((packed)) name##_t;
#elif defined(CLOG_COMPILER_MSVC)
#define CLOG_PACKED_STRUCT(name, stat)                         \
    __pragma(pack(push, 1)) typedef struct name stat name##_t; \
    __pragma(pack(pop))
#else
#error "Compiler Not Supported Now"
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
    CLOG_NOT_COMPLETED = 11, /* operation not completed */
    CLOG_ABNORMAL_STATE = 12, /* abnormal state */
    CLOG_BUSY = 13, /* resource busy */
    CLOG_TIMEOUT = 14, /* operation timeout */
} clog_res_e;

typedef enum clog_level {
    CLOG_LEVEL_OFF = 0, /* log is not allowed to write */
    CLOG_LEVEL_TRACE = 1, /* value: 1(1 << 0) */
    CLOG_LEVEL_DEBUG = 2, /* value: 2(1 << 1) */
    CLOG_LEVEL_INFO = 3, /* value: 4(1 << 2) */
    CLOG_LEVEL_WARN = 4, /* value: 8(1 << 3) */
    CLOG_LEVEL_ERROR = 5, /* value: 16(1 << 4) */
    CLOG_LEVEL_FETAL = 6, /* value: 32(1 << 5) */
} clog_level_e;

#define CLOG_LEVEL_NUM 6
#define CLOG_LEVEL_ALL 63 /* 1 | 2 | 4 | 8 | 16 | 32 */

#ifdef CLOG_COMPILER_MSVC
#pragma pack(push, 1)
#endif
typedef struct clog_item {
    uint32_t recorder[CASCA_LOG_TARGET_RECORDER_MAX_NUM];
    char process[CASCA_LOG_PROCESS_NAME_MAX_SIZE];
    char module[CASCA_LOG_MODULE_NAME_MAX_SIZE];
    char filename[CASCA_LOG_FILENAME_MAX_SIZE];
    uint64_t tid;
    uint32_t line;
    uint32_t level;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
    uint64_t timestamp;
    uint16_t length;
    char content[CASCA_LOG_SINGLE_LOG_MAX_SIZE];
}
#if defined(CLOG_COMPILER_CLANG) || defined(CLOG_COMPILER_GCC)
__attribute__((packed))
#endif
clog_item_t;
#ifdef CLOG_COMPILER_MSVC
#pragma pack(pop)
#endif

typedef struct clog_item_wrapper {
    const char *process;
    const char *module;
    const char *filename;
    const char *function;
    const char *fmt;
    va_list args;
    clog_item_t *log;
} clog_item_wrapper_t;

typedef struct clog_module {
    char name[CASCA_LOG_MODULE_NAME_MAX_SIZE];
    uint32_t level;
    uint32_t num;
    uint32_t recorders[CASCA_LOG_TARGET_RECORDER_MAX_NUM];
} clog_module_t;

clog_res_e clog_err_to_res(int err);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_BASE_H */
