/**
 * @auther Polaris
 * @date  2025/9/8
 */
#include "clog_secure_func.h"
#include <string.h>
#include "casca_log_defines.h"

clog_res_e clog_memset(void* dest, size_t size, char padding, size_t count)
{
    CLOG_RET_IF_NULL(dest, CLOG_INVALID_PARAM);
    CLOG_RET_IF((count == 0) || (count > size), CLOG_INVALID_PARAM);
    (void)memset(dest, padding, count);
    return CLOG_SUCCESS;
}

clog_res_e clog_memcpy(void* dest, size_t size, const void* src, size_t count)
{
    CLOG_RET_IF((dest == NULL) || (src == NULL), CLOG_INVALID_PARAM);
    CLOG_RET_IF((count == 0) || (count > size), CLOG_INVALID_PARAM);
    (void)memcpy(dest, src, count);
    return CLOG_SUCCESS;
}

char* clog_str_dup(const char* str)
{
    CLOG_RET_IF_NULL(str, NULL);
    const size_t len = strlen(str);
    char* dup = clog_malloc(len + 1);
    CLOG_RET_IF_NULL(dup, NULL);
    clog_memcpy(dup, len + 1, str, len);
    dup[len] = '\0';
    return dup;
}
