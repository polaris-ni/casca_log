/**
 * @author Polaris
 * @date  2025/9/8
 */
#ifndef CASCA_LOG_CLOG_SECURE_FUNC_H
#define CASCA_LOG_CLOG_SECURE_FUNC_H

#include <string.h>
#include "casca_log_base.h"
#include "clog_hooks.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * set memory to padding
 * @param dest destination memory
 * @param size size of memory
 * @param padding padding char
 * @param count count of padding
 * @return result
 */
clog_res_e clog_memset(void* dest, size_t size, char padding, size_t count);

/**
 * copy memory
 * @param dest destination memory
 * @param size size of memory
 * @param src source memory
 * @param count count of memory
 * @return result
 */
clog_res_e clog_memcpy(void* dest, size_t size, const void* src, size_t count);

/**
 * duplicate string
 * @param str string
 * @return result, NULL if str is NULL
 */
char* clog_strdup(const char* str);

/**
 * duplicate the first n bytes of string
 * example:
 *      "clog, hello, world": start -> 'h', end -> 'r'
 *      result: "hello, wor"
 * @param start start char of string that to be copied
 * @param num the num to be duplicated
 * @return result
 */
static char* clog_strndup(const char* start, const size_t num)
{
    CLOG_RET_IF(num == 0, NULL);
    CLOG_RET_IF_NULL(start, NULL);
    char* tmp = (char*)clog_malloc(num + 1);
    CLOG_RET_IF_NULL(tmp, NULL);
    CLOG_IGNORE_RES(clog_memcpy(tmp, num + 1, start, num));
    tmp[num] = '\0';
    return tmp;
}

/**
 * compare strings
 * if str1 and str2 are all NULL, 0 will be returned
 * if one of str1 and str2 is NULL, NULL string is considered as smaller
 * @param str1 string, nullable
 * @param str2 another string, nullable
 * @param num the num to be compared
 * @return 0 if equal, -1 if str1 < str2, 1 if str1 > str2
 */
static int32_t clog_strncmp(const char* str1, const char* str2, const size_t num)
{
    if (str1 == NULL && str2 == NULL) {
        return 0;
    }
    if (str1 == NULL || str2 == NULL) {
        return str1 == NULL ? -1 : 1;
    }
    return strncmp(str1, str2, num);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_SECURE_FUNC_H */
