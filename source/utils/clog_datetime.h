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

/**
 * get current datatime
 * @param datetime #clog_datetime_t
 */
void clog_datetime_now(clog_datetime_t *datetime);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_DATETIME_H */
