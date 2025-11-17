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

/* Microsoft Visual C++ */
#if defined(_MSC_VER)
#ifndef __clang__
#define CLOG_COMPILER_MSVC 1
#define CLOG_COMPILER_NAME "Microsoft Visual C++"
#define CLOG_COMPILER_VERSION _MSC_VER
#endif
#endif

/* GCC (GNU Compiler Collection) */
#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER) && !defined(__CUDACC__)
#define CLOG_COMPILER_GCC 1
#define CLOG_COMPILER_NAME "GCC"
#define CLOG_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

/* Clang */
#if defined(__clang__) && !defined(__INTEL_CLANG_COMPILER)
#define CLOG_COMPILER_CLANG 1
#define CLOG_COMPILER_NAME "Clang"
#define CLOG_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#endif

/* undefined */
#if !CLOG_COMPILER_MSVC && !CLOG_COMPILER_GCC && !CLOG_COMPILER_CLANG
#error "Compiler Not Supported Now"
#endif

#ifndef CLOG_COMPILER_NAME
#error "Complier Name Not Defined"
#endif

#ifndef CLOG_COMPILER_VERSION
#error "Complier Version Not Defined"
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_PLATFORM_H */
