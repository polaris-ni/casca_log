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

static inline int clog_datetime_is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int clog_datetime_days_in_month(int year, int month)
{
    const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && clog_datetime_is_leap_year(year)) return 29;
    return days[month - 1];
}

uint64_t clog_timestamp_of_datetime(const clog_datetime_t *datetime)
{
    CLOG_RET_IF_NULL(datetime, 0);
    uint64_t total_days = 0;

    for (uint16_t y = 1970; y < datetime->year; y++) {
        total_days += clog_datetime_is_leap_year(y) ? 366LL : 365LL;
    }

    for (uint8_t m = 1; m < datetime->month; m++) {
        total_days += clog_datetime_days_in_month(datetime->year, m);
    }

    total_days += (datetime->day - 1);

    uint64_t timestamp_ms = total_days * 86400000LL;
    timestamp_ms += datetime->hour * 3600000LL;
    timestamp_ms += datetime->minute * 60000LL;
    timestamp_ms += datetime->second * 1000LL;
    timestamp_ms += datetime->millisecond;
    return timestamp_ms;
}
