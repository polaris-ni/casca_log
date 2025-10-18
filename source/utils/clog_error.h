/**
 * @author polaris
 * @date  2025/10/18
 */
#ifndef CASCA_LOG_CLOG_ERROR_H
#define CASCA_LOG_CLOG_ERROR_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif


#define CLOG_RET_IF_X(cond, ret, msg, ...)            \
    do {                                              \
        if (cond) {                                   \
            clog_err_append_line(msg, ##__VA_ARGS__); \
            return ret;                               \
        }                                             \
    } while (0)

#define CLOG_CLEAN_RET_IF_X(cond, clean, ret, msg, ...) \
    do {                                                \
        if (cond) {                                     \
            clog_err_append_line(msg, ##__VA_ARGS__);   \
            (clean);                                    \
            return ret;                                 \
        }                                               \
    } while (0)

#define CLOG_RET_IF_NULL_X(ptr, ret, msg, ...)        \
    do {                                              \
        if ((ptr) == NULL) {                          \
            clog_err_append_line(msg, ##__VA_ARGS__); \
            return ret;                               \
        }                                             \
    } while (0)

#define CLOG_CLEAN_RET_IF_NULL_X(ptr, clean, ret, msg, ...) \
    do {                                                    \
        if ((ptr) == NULL) {                                \
            clog_err_append_line(msg, ##__VA_ARGS__);       \
            (clean);                                        \
            return ret;                                     \
        }                                                   \
    } while (0)

#define CLOG_RET_IF_FAILED_X(ret, msg, ...)           \
    do {                                              \
        if ((ret) != CLOG_SUCCESS) {                  \
            clog_err_append_line(msg, ##__VA_ARGS__); \
            return ret;                               \
        }                                             \
    } while (0)

#define CLOG_CLEAN_RET_IF_FAILED_X(ret, clean, msg, ...) \
    do {                                                 \
        if ((ret) != CLOG_SUCCESS) {                     \
            clog_err_append_line(msg, ##__VA_ARGS__);    \
            (clean);                                     \
            return ret;                                  \
        }                                                \
    } while (0)


/**
 * clear error message
 */
void clog_err_clear(void);

/**
 * set err msg, will clear previous error message even if set failed
 * @param fmt message format, nonnull
 * @param ... var
 */
void clog_err_set(const char* fmt, ...);

/**
 * append error message
 * @param fmt message format, nonnull
 * @param ... var
 */
void clog_err_append(const char* fmt, ...);

/**
 * append error message with line
 * @param fmt message format, nonnull
 * @param ... var
 */
void clog_err_append_line(const char* fmt, ...);

/**
 * get error message, it will never return NULL, safe to print
 * @return error message, return string "NULL" if context is NULL
 */
const char* clog_err_get(void);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_ERROR_H */
