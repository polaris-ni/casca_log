/**
 * @auther Polaris
 * @date  2025/9/8
 */
#ifndef CASCA_LOG_CLOG_SECURE_FUNC_H
#define CASCA_LOG_CLOG_SECURE_FUNC_H

#include "casca_log.h"
#include "clog_hooks.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

clog_res_e clog_memset(void* dest, size_t size, char padding, size_t count);

clog_res_e clog_memcpy(void* dest, size_t size, const void* src, size_t count);

char* clog_str_dup(const char* str);

/**
 * duplicate the first n bytes of string\n
 * example:\n
 *      "clog, hello, world": start -> 'h', end -> 'r'\n
 *      result: "hello, wor"
 * @param start start char of string that to be copied
 * @param num the num to be duplicated
 * @return result
 */
static char* clog_config_str_n_dup(const char* start, const size_t num)
{
    CLOG_RET_IF(num == 0, NULL);
    CLOG_RET_IF_NULL(start, NULL);
    char* tmp = clog_malloc(num + 1);
    CLOG_RET_IF_NULL(tmp, NULL);
    (void)clog_memcpy(tmp, num + 1, start, num);
    tmp[num] = '\0';
    return tmp;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_SECURE_FUNC_H */
