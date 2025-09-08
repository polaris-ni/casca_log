/**
 * @auther Polaris
 * @date  2025/9/8
 */
#include "clog_secure_func.h"

#include <string.h>

clog_res_e clog_memset(void* dest, size_t size, char padding, size_t count)
{
    CLOG_RET_IF_NULL(dest, CLOG_INVALID_PARAM);
    CLOG_RET_IF((count == 0) || (count > size), CLOG_INVALID_PARAM);
    (void)memset(dest, padding, count);
    return CLOG_SUCCESS;
}
