/**
 * @author Polaris
 * @date  2025/11/18
 */
#ifndef CASCA_LOG_BUILD_TEST_UTIL_H
#define CASCA_LOG_BUILD_TEST_UTIL_H

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <unordered_map>
#include "casca_log_base.h"
#include "clog_hooks.h"

class CLogMemoryInfo
{
    void* ptr;
    size_t size;
    const char* file;
    int line;
    const char* func;
    std::thread::id tid;

public:
    CLogMemoryInfo() : ptr(nullptr), size(0), file(nullptr), line(0), func(nullptr) {}

    CLogMemoryInfo(void* ptr, size_t size, const char* file, int line, const char* func, std::thread::id tid) :
        ptr(ptr), size(size), file(file), line(line), func(func), tid(tid)
    {
    }

    CLogMemoryInfo(const CLogMemoryInfo& other) = default;

    ~CLogMemoryInfo()
    {
        ptr = nullptr;
        size = 0;
        file = nullptr;
        line = 0;
        func = nullptr;
    }

    [[nodiscard]] std::shared_ptr<std::string> info() const;
};

class CLogMemLeakDetect
{
    static std::unordered_map<void*, CLogMemoryInfo> memory;

public:
    static void clog_allocate_callback(uintptr_t trace, const char* file, const char* function, int line, size_t size,
                                       void* ptr);

    static void clog_deallocate_callback(uintptr_t trace, const char* file, const char* function, int line, void* ptr);

    static void start(clog_allocator_f allocator, clog_deallocator_f deallocator);

    static void end();
};

#endif /* CASCA_LOG_BUILD_TEST_UTIL_H */
