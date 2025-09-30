/**
 * @author Polaris
 * @date  2025/9/9
 */
#ifndef CASCA_LOG_CASCA_LOG_DEFINES_H
#define CASCA_LOG_CASCA_LOG_DEFINES_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include "casca_log_config.h"
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
    do {                       \
        if (cond) {            \
            return;            \
        }                      \
    } while (0)

#define CLOG_RET_VOID_IF_NULL(ptr) CLOG_RET_VOID_IF((ptr) == NULL)

#define CLOG_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define CLOG_SAFE_FREE(mem)          \
    do {                             \
        if (mem != NULL) {           \
            clog_free((void*)(mem)); \
            (mem) = NULL;            \
        }                            \
    } while (0)

#define CLOG_RET_IF_X(cond, ret, msg, ...)    \
    do {                                      \
        if (cond) {                           \
            clog_err_set(msg, ##__VA_ARGS__); \
            return ret;                       \
        }                                     \
    } while (0)

#define CLOG_RET_IF_NULL_X(ptr, ret, msg, ...) \
    do {                                       \
        if ((ptr) == NULL) {                   \
            clog_err_set(msg, ##__VA_ARGS__);  \
            return ret;                        \
        }                                      \
    } while (0)

#ifdef CASCA_LOG_DEBUG
#include <assert.h>
#define CLOG_ASSERT(cond) assert(cond)
#else
#define CLOG_ASSERT(cond)
#endif

#define CLOG_UNUSED_VAR(x) (void)x

typedef enum clog_res {
    CLOG_SUCCESS = 0, /* exec success */
    CLOG_FAIL = 1, /* exec failed */
    CLOG_NOT_SUPPORTED = 2, /* operation not supported */
    CLOG_INVALID_PARAM = 3, /* invalid param */
    CLOG_ERROR_FORMAT = 4, /* error format */
    CLOG_NO_MEMORY = 5, /* no memory, malloc failed */
    CLOG_TARGET_NOT_FOUND = 6, /* something not found */
} clog_res_e;

typedef enum clog_config_item_type {
    CLOG_CONFIG_ITEM_TYPE_INT = 0,
    CLOG_CONFIG_ITEM_TYPE_UINT,
    CLOG_CONFIG_ITEM_TYPE_FLOAT,
    CLOG_CONFIG_ITEM_TYPE_CHAR,
    CLOG_CONFIG_ITEM_TYPE_STRING,
    CLOG_CONFIG_ITEM_TYPE_BOOL,
    CLOG_CONFIG_ITEM_TYPE_INVALID,
} clog_config_item_type_e;

typedef struct clog_item {
    const char* filepath;
    const char* filename;
    const char* function;
    const char* module;
    unsigned long long tid;
    unsigned int line;
    clog_level_e level;
    struct {
        unsigned short year;
        unsigned char month;
        unsigned char day;
        unsigned char hour;
        unsigned char minute;
        unsigned char second;
        unsigned short millisecond;
    };
    const char* content;
} clog_item_t;

typedef struct clog_context clog_context_t;

typedef struct clog_config_item clog_config_item_t;

struct clog_config_item {
    const char* key;
    clog_config_item_type_e type;
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
    clog_config_group_t* root;
    clog_level_e level;
} clog_config_t;

/**
 * placeholder
 * @param item log item, contains log info and content, it should never be NULL
 * @param buf the buffer to store log
 * @param size buffer size
 * @return size_t write size
 */
typedef size_t (*clog_placeholder_f)(const clog_item_t* item, char* buf, size_t size);

typedef struct clog_placeholder {
    const char* name;
    clog_placeholder_f func;
    struct clog_placeholder* next;
} clog_placeholder_t;

struct clog_context {
    const char* process;
    clog_config_t config;
    struct {
        clog_placeholder_t placeholder;
        struct {
            struct {
                const char* trace;
                const char* debug;
                const char* info;
                const char* warn;
                const char* error;
                const char* fetal;
            } tag;
        } level;
    } formatter;
};

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_DEFINES_H */
