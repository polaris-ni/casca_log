/**
 * @author Polaris
 * @date  2025/11/18
 */
#ifndef CASCA_LOG_TEST_UTIL_H
#define CASCA_LOG_TEST_UTIL_H

#include <array>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <unordered_map>
#include "casca_log_base.h"
#include "clog_hooks.h"

namespace CLogTest
{

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

        [[nodiscard]] std::shared_ptr<std::string> info(const std::string_view& prefix) const;
    };

    class CLogMemLeakDetect
    {
        static std::unordered_map<void*, CLogMemoryInfo> memory;
        static std::mutex mutex;
        static std::vector<std::shared_ptr<std::string>> logs;

    public:
        static void clog_allocate_callback(uintptr_t trace, const char* file, const char* function, int line,
                                           size_t size, void* ptr);

        static void clog_deallocate_callback(uintptr_t trace, const char* file, const char* function, int line,
                                             void* ptr);

        static void start(clog_allocator_f allocator, clog_deallocator_f deallocator);

        static void end();
    };

    template <std::size_t NP, std::size_t NC, typename T>
    class CLogProducerConsumerDataHolder
    {
        std::array<std::unordered_map<T, std::size_t>, NP> produced_values{};
        std::array<std::vector<T>, NC> consumed_values{};

    public:
        std::size_t producer_num = NP;
        std::size_t num_per_producer = 0;
        std::size_t consumer_num = NC;
        std::size_t num_per_consumer = 0;

        CLogProducerConsumerDataHolder(std::size_t num_per_producer, std::size_t num_per_consumer) :
            num_per_producer(num_per_producer), num_per_consumer(num_per_consumer)
        {
        }


        void Produce(std::size_t who, T what)
        {
            auto& map = produced_values.at(who);
            auto res = map.find(what);
            if (res == map.end()) {
                map[what] = 1;
            } else {
                res->second = res->second + 1;
            }
        }

        void Consume(std::size_t who, T what)
        {
            auto& vec = consumed_values.at(who);
            vec.push_back(what);
        }

        void Validate(std::size_t expected_num)
        {
            std::unordered_map<T, std::size_t> total;
            std::size_t num = 0;
            for (auto& map : produced_values) {
                for (auto element : map) {
                    auto res = total.find(element.first);
                    if (res == total.end()) {
                        total[element.first] = element.second;
                    } else {
                        res->second += element.second;
                    }
                    num += element.second;
                }
            }

            if (expected_num != 0) {
                ASSERT_EQ(expected_num, num);
            }

            for (auto& vec : consumed_values) {
                for (T element : vec) {
                    auto res = total.find(element);
                    ASSERT_NE(res, total.end()) << "Value not found: " << element;
                    ASSERT_NE(res->second, 0) << "Value num not meet: " << element;
                    res->second -= 1;
                }
            }

            for (auto [value, cnt] : total) {
                ASSERT_EQ(cnt, 0) << "Value not consumed: " << value << "(" << cnt << " left)";
            }
        }
    };

    int GenerateRandomNumber(int min, int max);

} // namespace CLogTest
#endif /* CASCA_LOG_TEST_UTIL_H */
