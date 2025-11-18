/**
 * @auther Polaris
 * @date  2025/11/18
 */

#include "test_util.h"

std::unordered_map<void*, CLogMemoryInfo> CLogMemLeakDetect::memory;

std::shared_ptr<std::string> CLogMemoryInfo::info() const
{
    std::stringstream ss;
    ss << "ptr: " << ptr << ", size: " << size << ", file: " << file << ", line: " << line << ", func: " << func
       << ", tid: " << tid;
    return std::make_shared<std::string>(ss.str());
}

void CLogMemLeakDetect::clog_allocate_callback(uintptr_t trace, const char* file, const char* function, int line,
                                               size_t size, void* ptr)
{
    CLOG_UNUSED_VAR(trace);
    if (ptr == nullptr) {
        return;
    }
    const CLogMemoryInfo info(ptr, size, file, line, function, std::this_thread::get_id());
    const auto tmp = memory.find(ptr);
    ASSERT_EQ(tmp, memory.end()) << "Memory info is already existed, current is " << info.info()->c_str()
                                 << ", previous info is " << tmp->second.info()->c_str();
    memory[ptr] = info;
}

void CLogMemLeakDetect::clog_deallocate_callback(uintptr_t trace, const char* file, const char* function, int line,
                                                 void* ptr)
{
    CLOG_UNUSED_VAR(trace);
    if (ptr == nullptr) {
        return;
    }
    const auto tmp = memory.find(ptr);
    ASSERT_NE(tmp, memory.end()) << "Memory info not found, ptr: " << ptr << ", file: " << file << ", line: " << line
                                 << ", func: " << function;
    memory.erase(tmp);
}

void CLogMemLeakDetect::start(clog_allocator_f allocator, clog_deallocator_f deallocator)
{
    memory.clear();
    clog_register_memory_hook_func(allocator, deallocator, clog_allocate_callback, clog_deallocate_callback);
}

void CLogMemLeakDetect::end()
{
    for (const auto& [_, info] : memory) {
        std::cout << "Memory leak info: " << info.info()->c_str() << std::endl;
    }
    ASSERT_TRUE(memory.empty());
    clog_register_memory_hook_func(nullptr, nullptr, nullptr, nullptr);
}
