/**
 * @author polaris
 * @date  2025/10/18
 */
#ifndef CASCA_LOG_CLOG_ERROR_H
#define CASCA_LOG_CLOG_ERROR_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef CLOG_FILENAME
#ifndef __FILE_NAME__
static const char* clog_get_filename(const char* fullname, int len)
{
    if (len <= 1) {
        return fullname;
    }
    int i = len - 2;
    while (i >= 0) {
        if (fullname[i] == '/' || fullname[i] == '\\') {
            return fullname + i + 1;
        }
        --i;
    }
    return fullname;
}
#define CLOG_FILENAME clog_get_filename(__FILE__, (int)sizeof(__FILE__))
#else
#define CLOG_FILENAME __FILE_NAME__
#endif
#endif

#define CLOG_STR_TMP(x) #x
#define CLOG_STR(x) CLOG_STR_TMP(x)


#define CLOG_RET_IF_X(cond, ret, msg, ...)    \
    do {                                      \
        if (cond) {                           \
            CLOG_ERR_ADD(msg, ##__VA_ARGS__); \
            return ret;                       \
        }                                     \
    } while (0)

#define CLOG_CLEAN_RET_IF_X(cond, clean, ret, msg, ...) \
    do {                                                \
        if (cond) {                                     \
            CLOG_ERR_ADD(msg, ##__VA_ARGS__);           \
            (clean);                                    \
            return ret;                                 \
        }                                               \
    } while (0)

#define CLOG_RET_IF_NULL_X(ptr, ret, msg, ...) \
    do {                                       \
        if ((ptr) == NULL) {                   \
            CLOG_ERR_ADD(msg, ##__VA_ARGS__);  \
            return ret;                        \
        }                                      \
    } while (0)

#define CLOG_CLEAN_RET_IF_NULL_X(ptr, clean, ret, msg, ...) \
    do {                                                    \
        if ((ptr) == NULL) {                                \
            CLOG_ERR_ADD(msg, ##__VA_ARGS__);               \
            (clean);                                        \
            return ret;                                     \
        }                                                   \
    } while (0)

#define CLOG_RET_IF_FAILED_X(ret, msg, ...)   \
    do {                                      \
        if ((ret) != CLOG_SUCCESS) {          \
            CLOG_ERR_ADD(msg, ##__VA_ARGS__); \
            return ret;                       \
        }                                     \
    } while (0)

#define CLOG_CLEAN_RET_IF_FAILED_X(ret, clean, msg, ...) \
    do {                                                 \
        if ((ret) != CLOG_SUCCESS) {                     \
            CLOG_ERR_ADD(msg, ##__VA_ARGS__);            \
            (clean);                                     \
            return ret;                                  \
        }                                                \
    } while (0)

#define CLOG_ERR_ADD(msg, ...) clog_err_put(CLOG_FILENAME, __LINE__, msg, ##__VA_ARGS__)

/**
 * init err msg
 * @param num max num of saved logs
 * @param size each buffer size of log
 */
void clog_err_setup(unsigned short num, unsigned short size);

/**
 * record a log
 * @param file file name
 * @param line line
 * @param fmt msg format
 * @param ... vararg
 */
void clog_err_put(const char* file, int line, const char* fmt, ...);

/**
 * get raw error message
 * @param num the num of message
 * @return error message
 */
const char** clog_err_get(unsigned int* num);

/**
 * print log to buf
 * @param buf buffer
 * @param size size of buffer
 * @param separator separator of line, using new line if separator is NULL
 */
void clog_err_print(char* buf, unsigned int size, const char* separator);

/**
 * clear error message
 */
void clog_err_clear(void);

/**
 * release log buffer that malloc in #clog_err_setup
 */
void clog_err_cleanup();

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_ERROR_H */
