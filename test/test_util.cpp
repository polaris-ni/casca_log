/**
 * @auther Polaris
 * @date  2025/11/18
 */

#include "test_util.h"

#include <random>

namespace CLogTest
{
    std::unordered_map<void *, CLogMemoryInfo> CLogMemLeakDetect::memory;
    std::vector<std::shared_ptr<std::string>> CLogMemLeakDetect::logs;
    std::mutex CLogMemLeakDetect::mutex;

    std::shared_ptr<std::string> CLogMemoryInfo::info(const std::string_view &prefix) const
    {
        std::stringstream ss;
        ss << prefix;
        ss << "ptr: " << ptr << ", size: " << size << ", allocated at " << file << ":" << line << ", func: " << func
           << ", tid: " << tid;
        return std::make_shared<std::string>(ss.str());
    }

    void CLogMemLeakDetect::clog_allocate_callback(uintptr_t trace, const char *file, const char *function, int line,
                                                   size_t size, void *ptr)
    {
        CLOG_UNUSED_VAR(trace);
        if (ptr == nullptr) {
            return;
        }
        const CLogMemoryInfo info(ptr, size, file, line, function, std::this_thread::get_id());
        std::lock_guard lock(mutex);
        const auto tmp = memory.find(ptr);
        if (tmp != memory.end()) {
            logs.push_back(info.info("[ Memory Allocated ]\t"));
            for (const auto &str : logs) {
                std::cout << *str << std::endl;
            }
        }
        ASSERT_EQ(tmp, memory.end()) << "Memory info is already existed: " << info.info("CURRENT\t")->c_str() << " "
                                     << tmp->second.info("PREVIOUS\t")->c_str();
        memory[ptr] = info;
        logs.push_back(info.info("[ Memory Allocated ]\t"));
    }

    void CLogMemLeakDetect::clog_deallocate_callback(uintptr_t trace, const char *file, const char *function, int line,
                                                     void *ptr)
    {
        CLOG_UNUSED_VAR(trace);
        if (ptr == nullptr) {
            return;
        }
        std::lock_guard lock(mutex);
        const auto tmp = memory.find(ptr);
        ASSERT_NE(tmp, memory.end()) << "Memory info not found, ptr: " << ptr << ", file: " << file
                                     << ", line: " << line << ", func: " << function;
        logs.push_back(tmp->second.info("[ Memory Deallocated ]\t"));
        memory.erase(tmp);
    }

    void CLogMemLeakDetect::start(clog_allocator_f allocator, clog_deallocator_f deallocator)
    {
        memory.clear();
        logs.clear();
        clog_register_memory_hook_func(allocator, deallocator, clog_allocate_callback, clog_deallocate_callback);
    }

    void CLogMemLeakDetect::end()
    {
        for (const auto &[_, info] : memory) {
            std::cout << info.info("[ LEAK ]\t")->c_str() << std::endl;
        }
        ASSERT_TRUE(memory.empty());
        GTEST_LOG_(INFO) << "No Memory Leak Detected";
        clog_register_memory_hook_func(nullptr, nullptr, nullptr, nullptr);
        memory.clear();
        logs.clear();
    }

    int GenerateRandomNumber(int min, int max)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution dis(min, max);
        return dis(gen);
    }
} // namespace CLogTest
