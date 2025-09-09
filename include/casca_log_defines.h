/**
 * @auther Polaris
 * @date  2025/9/9
 */
#ifndef CASCA_LOG_CASCA_LOG_DEFINES_H
#define CASCA_LOG_CASCA_LOG_DEFINES_H

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

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_DEFINES_H */
