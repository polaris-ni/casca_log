/**
 * @author Polaris
 * @date 2026/2/23
 */

#include "clog_datetime.h"

#include "casca_log_base.h"
#ifdef CLOG_PLATFORM_WINDOWS
#include <Windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

void clog_datetime_now(clog_datetime_t *datetime)
{
    CLOG_RET_VOID_IF_NULL(datetime);
#ifdef CLOG_PLATFORM_WINDOWS
    SYSTEMTIME st;
    GetLocalTime(&st);
    datetime->year = st.wYear;
    datetime->month = (uint8_t)st.wMonth;
    datetime->day = (uint8_t)st.wDay;
    datetime->hour = (uint8_t)st.wHour;
    datetime->minute = (uint8_t)st.wMinute;
    datetime->second = (uint8_t)st.wSecond;
    datetime->millisecond = st.wMilliseconds;
#else
    struct timeval tv;
    CLOG_IGNORE_RES(gettimeofday(&tv, NULL));
    struct tm tm_info;
    CLOG_IGNORE_RES(localtime_r(&tv.tv_sec, &tm_info));
    datetime->year = tm_info.tm_year + 1900;
    datetime->month = tm_info.tm_mon + 1;
    datetime->day = tm_info.tm_mday;
    datetime->hour = tm_info.tm_hour;
    datetime->minute = tm_info.tm_min;
    datetime->second = tm_info.tm_sec;
    datetime->millisecond = tv.tv_usec / 1000;
#endif
}

uint64_t clog_timestamp_ms()
{
#ifdef CLOG_PLATFORM_WINDOWS
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER ui;
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;

    return (ui.QuadPart - 116444736000000000ULL) / 10000ULL;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
#endif
}
