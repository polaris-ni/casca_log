/**
 * @author Polaris
 * @date 2026/2/24
 */

#ifndef CASCA_LOG_CLOG_INTERPOLATOR_DEFAULT_PLACEHOLDER_H
#define CASCA_LOG_CLOG_INTERPOLATOR_DEFAULT_PLACEHOLDER_H

#include <stdio.h>
#include "casca_log_base.h"
#include "casca_log_config.h"
#include "clog_datetime.h"
#include "clog_error.h"
#include "clog_file_system.h"
#include "clog_hashmap.h"
#include "clog_interpolator.h"
#include "clog_secure_func.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_DECLARE_INTERPOLATOR_DEFAULT_DATETIME_PLACEHOLDER(name, fmt, property)      \
    static int clog_interpolator_placeholder_##name(void *param, char *buf, size_t size) \
    {                                                                                    \
        CLOG_UNUSED_VAR(param);                                                          \
        clog_datetime_t now;                                                             \
        clog_datetime_now(&now);                                                         \
        const int ret = snprintf(buf, size, fmt, property);                              \
        CLOG_RET_IF(ret <= 0, CLOG_FAIL);                                                \
        CLOG_RET_IF(ret >= size, -CLOG_NO_MEMORY);                                       \
        return ret;                                                                      \
    }

CLOG_DECLARE_INTERPOLATOR_DEFAULT_DATETIME_PLACEHOLDER(year, "%04u", now.year)
CLOG_DECLARE_INTERPOLATOR_DEFAULT_DATETIME_PLACEHOLDER(month, "%02u", now.month)
CLOG_DECLARE_INTERPOLATOR_DEFAULT_DATETIME_PLACEHOLDER(day, "%02u", now.day)
CLOG_DECLARE_INTERPOLATOR_DEFAULT_DATETIME_PLACEHOLDER(hour, "%02u", now.hour)
CLOG_DECLARE_INTERPOLATOR_DEFAULT_DATETIME_PLACEHOLDER(minute, "%02u", now.minute)
CLOG_DECLARE_INTERPOLATOR_DEFAULT_DATETIME_PLACEHOLDER(second, "%02u", now.second)
CLOG_DECLARE_INTERPOLATOR_DEFAULT_DATETIME_PLACEHOLDER(millisecond, "%03u", now.millisecond)
#undef CLOG_DECLARE_INTERPOLATOR_DEFAULT_DATETIME_PLACEHOLDER

static int clog_interpolator_placeholder_cwd(void *param, char *buf, size_t size)
{
    CLOG_UNUSED_VAR(param);
    char path[CASCA_LOG_FILEPATH_MAX_SIZE] = {0};
    clog_res_e ret = clog_cwd(path, sizeof(path));
    CLOG_RET_IF_X(ret != CLOG_SUCCESS, -ret, "clog_cwd failed, ret = %u", ret);
    ret = clog_strcpy(buf, size, path);
    CLOG_RET_IF_X(ret != CLOG_SUCCESS, -ret, "clog_strcpy failed, ret = %u", ret);
    return (int)strlen(path);
}

static int clog_interpolator_placeholder_path_separator(void *param, char *buf, size_t size)
{
    CLOG_UNUSED_VAR(param);
    CLOG_UNUSED_VAR(size);
#ifdef CLOG_PLATFORM_WINDOWS
    buf[0] = '\\';
#else
    buf[0] = '/';
#endif
    return 1;
}

static clog_hashmap_t *clog_interpolator_default_placeholders()
{
    clog_hashmap_t *map =
        clog_hashmap_create(0, sizeof(clog_placeholder_handler_f), clog_hashmap_string_dup, clog_hashmap_string_free,
                            NULL, NULL, clog_hashmap_string_cmp, clog_hashmap_string_size, 0);
    CLOG_RET_IF_NULL_X(map, NULL, "clog_hashmap_create failed");
    const char *default_placeholder_names[] = {
        "_year", "_month", "_day", "_hour", "_minute", "_second", "_millisecond", "_cwd", "_path_separator",
    };
    const clog_placeholder_handler_f default_placeholder_handlers[] = {
        clog_interpolator_placeholder_year,
        clog_interpolator_placeholder_month,
        clog_interpolator_placeholder_day,
        clog_interpolator_placeholder_hour,
        clog_interpolator_placeholder_minute,
        clog_interpolator_placeholder_second,
        clog_interpolator_placeholder_millisecond,
        clog_interpolator_placeholder_cwd,
        clog_interpolator_placeholder_path_separator,
    };
    CLOG_ASSERT(CLOG_ARRAY_SIZE(default_placeholder_names) == CLOG_ARRAY_SIZE(default_placeholder_handlers));
    const size_t default_placeholder_num = CLOG_ARRAY_SIZE(default_placeholder_names);
    for (size_t i = 0; i < default_placeholder_num; i++) {
        const clog_res_e ret = clog_hashmap_put(map, default_placeholder_names[i], &default_placeholder_handlers[i]);
        CLOG_CLEAN_RET_IF_X(ret != CLOG_SUCCESS, clog_hashmap_destroy(&map), NULL,
                            "add default log format placeholder %s failed, ret = %u", default_placeholder_names[i],
                            ret);
    }
    return map;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_INTERPOLATOR_DEFAULT_PLACEHOLDER_H */
