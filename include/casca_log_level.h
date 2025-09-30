/**
 * @author Polaris
 * @date  2025/9/2
 */
#ifndef CASCA_LOG_CASCA_LOG_LEVEL_H
#define CASCA_LOG_CASCA_LOG_LEVEL_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum clog_level {
    CLOG_LEVEL_OFF = 0, /* log is not allowed to write */
    CLOG_LEVEL_TRACE = 1, /* 1 << 0 */
    CLOG_LEVEL_DEBUG = 2, /* 1 << 1 */
    CLOG_LEVEL_INFO = 4, /* 1 << 2 */
    CLOG_LEVEL_WARN = 8, /* 1 << 3 */
    CLOG_LEVEL_ERROR = 16, /* 1 << 4 */
    CLOG_LEVEL_FETAL = 32, /* 1 << 5 */
} clog_level_e;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_LEVEL_H */
