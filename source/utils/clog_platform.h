/**
 * @author Polaris
 * @date  2025/9/10
 */
#ifndef CASCA_LOG_CLOG_PLATFORM_H
#define CASCA_LOG_CLOG_PLATFORM_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
#define CLOG_PLATFORM_WINDOWS
#elif defined(__APPLE__) || defined(__MACH__)
#define CLOG_PLATFORM_MACOS
#elif defined(__linux__) || defined(__linux)
#define CLOG_PLATFORM_LINUX
#elif defined(__unix__) || defined(unix)
#define CLOG_PLATFORM_UNIX
#else
#define CLOG_PLATFORM_UNKNOWN
#endif

#if defined(_WIN64) || defined(__x86_64__) || defined(__amd64__) || defined(__aarch64__) || defined(__LP64__)
#define CLOG_ARCH_64BIT
#else
#define CLOG_ARCH_32BIT
#endif


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_PLATFORM_H */
