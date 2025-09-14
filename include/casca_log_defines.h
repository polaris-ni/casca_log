/**
 * @auther Polaris
 * @date  2025/9/9
 */
#ifndef CASCA_LOG_CASCA_LOG_DEFINES_H
#define CASCA_LOG_CASCA_LOG_DEFINES_H

#include <stdbool.h>
#include <stddef.h>
#include "casca_log_level.h"

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
    do {                           \
        if (cond) {                \
            return;                \
        }                          \
    } while (0)

#define CLOG_RET_VOID_IF_NULL(ptr) CLOG_RET_VOID_IF((ptr) == NULL)

#define CLOG_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef enum clog_res {
    CLOG_SUCCESS = 0, /* exec success */
    CLOG_FAIL = 1, /* exec failed */
    CLOG_NOT_SUPPORTED = 2, /* operation not supported */
    CLOG_INVALID_PARAM = 3, /* invalid param */
} clog_res_e;

typedef enum clog_config_item_type {
    CLOG_CONFIG_ITEM_TYPE_INT = 0,
    CLOG_CONFIG_ITEM_TYPE_UINT,
    CLOG_CONFIG_ITEM_TYPE_FLOAT,
    CLOG_CONFIG_ITEM_TYPE_CHAR,
    CLOG_CONFIG_ITEM_TYPE_STRING,
    CLOG_CONFIG_ITEM_TYPE_BOOL,
    CLOG_CONFIG_ITEM_TYPE_INVALID,
} clog_config_item_type_t;

typedef struct clog_config_item clog_config_item_t;

struct clog_config_item {
    const char* key;
    clog_config_item_type_t type;
    union {
        int sint;
        unsigned int uint;
        char ch;
        char* str;
        bool flag;
        double f;
    } value;
    clog_config_item_t* next;
};

typedef struct clog_config_group clog_config_group_t;

struct clog_config_group {
    const char* name;
    clog_config_group_t* child;
    clog_config_group_t* sibling;
    clog_config_item_t* content;
};

typedef struct clog_config {
    clog_config_group_t* raw;
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
