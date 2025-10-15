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
#define CLOG_COMPILER_MSVC 1
#define CLOG_COMPILER_NAME "Microsoft Visual C++"
#define CLOG_COMPILER_VERSION _MSC_VER
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

/* Intel C++ Compiler (经典版) */
#if defined(__INTEL_COMPILER)
#define CLOG_COMPILER_ICC 1
#define CLOG_COMPILER_NAME "Intel C++ Compiler (Classic)"
#define CLOG_COMPILER_VERSION __INTEL_COMPILER
#endif

/* Intel oneAPI DPC++/C++ Compiler (based on LLVM/Clang) */
#if defined(__INTEL_LLVM_COMPILER) || defined(__INTEL_CLANG_COMPILER)
#define CLOG_COMPILER_ICX 1
#define CLOG_COMPILER_NAME "Intel oneAPI DPC++/C++ Compiler"
#ifdef __INTEL_LLVM_COMPILER
#define CLOG_COMPILER_VERSION __INTEL_LLVM_COMPILER
#else
#define CLOG_COMPILER_VERSION __INTEL_CLANG_COMPILER
#endif
#endif

/* NVIDIA CUDA Compiler (nvcc) */
#if defined(__CUDACC__)
#define CLOG_COMPILER_NVCC 1
#define CLOG_COMPILER_NAME "NVIDIA CUDA Compiler"
#define CLOG_COMPILER_VERSION __CUDACC_VER_MAJOR__ * 10000 + __CUDACC_VER_MINOR__ * 100 + __CUDACC_VER_BUILD__
#endif

/* 检测 Apple Clang (Clang Apple Branch) */
#if defined(__APPLE__) && defined(__clang__) && defined(__apple_build_version__)
#define CLOG_COMPILER_APPLE_CLANG 1
#undef CLOG_COMPILER_NAME
#define CLOG_COMPILER_NAME "Apple Clang"
#endif

/* undefined */
#if !CLOG_COMPILER_MSVC && !CLOG_COMPILER_GCC && !CLOG_COMPILER_CLANG && !CLOG_COMPILER_ICC && !CLOG_COMPILER_ICX && \
    !CLOG_COMPILER_NVCC
#define CLOG_COMPILER_UNKNOWN 1
#endif

#ifndef CLOG_COMPILER_NAME
#error "Complier Name is Not Defined"
#endif

#ifndef CLOG_COMPILER_VERSION
#error "Complier Version is Not Defined"
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_PLATFORM_H */
