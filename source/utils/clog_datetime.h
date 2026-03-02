/**
 * @author Polaris
 * @date 2026/2/23
 */

#ifndef CASCA_LOG_CLOG_DATETIME_H
#define CASCA_LOG_CLOG_DATETIME_H

#include <stdint.h>
#include "clog_platform.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifdef CLOG_COMPILER_MSVC
#pragma pack(push, 1)
#endif
typedef struct clog_datetime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
}
#if defined(CLOG_COMPILER_CLANG) || defined(CLOG_COMPILER_GCC)
__attribute__((packed))
#endif
clog_datetime_t;
#ifdef CLOG_COMPILER_MSVC
#pragma pack(pop)
#endif

#define CLOG_TIMEZONE_OF(offset, hour, minute) offset hour, offset minute

/**
 * get current local datetime
 * @param datetime #clog_datetime_t
 */
void clog_datetime_now(clog_datetime_t *datetime);

/**
 * get current local timestamp
 * @return timestamp in ms
 */
uint64_t clog_timestamp_ms();

/**
 * get timestamp of datetime
 * @param datetime #clog_datetime_t
 * @param tz_hour timezone hour part
 * @param tz_minute timezone minute part
 * @return timestamp in ms
 */
uint64_t clog_timestamp_of_datetime(const clog_datetime_t *datetime, int tz_hour, int tz_minute);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_DATETIME_H */
