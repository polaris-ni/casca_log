/**
 * @author Polaris
 * @date 2026/2/13
 */

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include "clog_file_system.h"
#include "test_util.h"

#ifdef CLOG_PLATFORM_WINDOWS
#include <io.h>
#define ACCESS _access
#else
#include <unistd.h>
#define ACCESS access
#endif

class CLogFileSystemComprehensiveTest : public testing::Test
{
protected:
    void SetUp() override
    {
        CLogTest::CLogMemLeakDetect::start(nullptr, nullptr);
    }

    void TearDown() override
    {
        CLogTest::CLogMemLeakDetect::end();
    }

    static std::string GetTempFilePath()
    {
        static int counter = 0;
#ifdef CLOG_PLATFORM_LINUX
        return "./temp_comprehensive_test_file_" + std::to_string(counter++) + ".txt";
#else
        return "temp_comprehensive_test_file_" + std::to_string(counter++) + ".txt";
#endif
    }

    static std::string GetTempDirPath()
    {
        static int counter = 0;
#ifdef CLOG_PLATFORM_LINUX
        return "./temp_comprehensive_test_dir_" + std::to_string(counter++);
#else
        return "temp_comprehensive_test_dir_" + std::to_string(counter++);
#endif
    }

    static void WriteTestDataToFile(const std::string &path, const std::string &data)
    {
        std::ofstream file(path, std::ios::out | std::ios::binary);
        if (file.is_open()) {
            file << data;
            file.close();
        }
    }

    static std::string ReadDataFromFile(const std::string &path)
    {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        std::string content;
        if (file.is_open()) {
            content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
        }
        return content;
    }
};

TEST_F(CLogFileSystemComprehensiveTest, OpenFileWithDifferentFlags)
{
    const std::string path = GetTempFilePath();

    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_CREATE, 0644);
    EXPECT_NE(file, nullptr);
    clog_file_close(&file);
    std::remove(path.c_str());

    file = clog_file_open(path.c_str(), CLOG_FILE_READ, 0644);
    EXPECT_EQ(file, nullptr);

    file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_READ | CLOG_FILE_CREATE, 0644);
    EXPECT_NE(file, nullptr);
    clog_file_close(&file);

    std::remove(path.c_str());
}

TEST_F(CLogFileSystemComprehensiveTest, OpenFileWithTruncateFlag)
{
    const std::string path = GetTempFilePath();
    const std::string initial_data = "Initial data that should be truncated";

    WriteTestDataToFile(path, initial_data);

    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_TRUNCATE, 0644);
    ASSERT_NE(file, nullptr);

    const std::string new_data = "New data";
    clog_file_write(file, new_data.c_str(), new_data.length());
    clog_file_close(&file);

    const std::string content = ReadDataFromFile(path);
    EXPECT_EQ(content, new_data);

    std::remove(path.c_str());
}

TEST_F(CLogFileSystemComprehensiveTest, OpenFileWithSyncFlag)
{
    const std::string path = GetTempFilePath();

    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_CREATE | CLOG_FILE_SYNC, 0644);
    EXPECT_NE(file, nullptr);

    if (file) {
        const std::string data = "Sync test data";
        const clog_res_e result = clog_file_write(file, data.c_str(), data.length());
        EXPECT_EQ(result, CLOG_SUCCESS);
        clog_file_close(&file);

        const std::string content = ReadDataFromFile(path);
        EXPECT_EQ(content, data);
    }

    std::remove(path.c_str());
}

TEST_F(CLogFileSystemComprehensiveTest, WriteFileVariousScenarios)
{
    const std::string path = GetTempFilePath();
    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_CREATE, 0644);
    ASSERT_NE(file, nullptr);

    const std::string data1 = "First line\n";
    clog_res_e result = clog_file_write(file, data1.c_str(), data1.length());
    EXPECT_EQ(result, CLOG_SUCCESS);

    const std::string data2 = "Second line\n";
    result = clog_file_write(file, data2.c_str(), data2.length());
    EXPECT_EQ(result, CLOG_SUCCESS);

    clog_file_close(&file);

    const std::string content = ReadDataFromFile(path);
    EXPECT_EQ(content, data1 + data2);

    std::remove(path.c_str());
}

TEST_F(CLogFileSystemComprehensiveTest, WriteFileEdgeCases)
{
    const std::string path = GetTempFilePath();
    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_CREATE, 0644);
    ASSERT_NE(file, nullptr);

    clog_res_e result = clog_file_write(file, "test", 0);
    EXPECT_EQ(result, CLOG_INVALID_PARAM);

    result = clog_file_write(file, nullptr, 10);
    EXPECT_EQ(result, CLOG_INVALID_PARAM);

    clog_file_close(&file);
    std::remove(path.c_str());
}

TEST_F(CLogFileSystemComprehensiveTest, ReadFileVariousScenarios)
{
    const std::string path = GetTempFilePath();
    const std::string test_data = "This is test data for reading\nLine 2\nLine 3";

    WriteTestDataToFile(path, test_data);

    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_READ, 0644);
    ASSERT_NE(file, nullptr);

    char buffer[256] = {0};
    size_t read_size = 0;
    const clog_res_e result = clog_file_read(file, buffer, test_data.length(), &read_size);
    EXPECT_EQ(result, CLOG_SUCCESS);
    EXPECT_EQ(read_size, test_data.length());
    EXPECT_EQ(std::string(buffer, read_size), test_data);

    clog_file_close(&file);
    std::remove(path.c_str());
}

TEST_F(CLogFileSystemComprehensiveTest, ReadFileSmallBuffer)
{
    const std::string path = GetTempFilePath();
    const std::string test_data = "Large amount of test data that won't fit in small buffer";

    WriteTestDataToFile(path, test_data);

    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_READ, 0644);
    ASSERT_NE(file, nullptr);

    char small_buffer[20] = {0};
    size_t read_size = 0;
    const clog_res_e result = clog_file_read(file, small_buffer, sizeof(small_buffer) - 1, &read_size);
    EXPECT_EQ(result, CLOG_SUCCESS);
    EXPECT_EQ(read_size, sizeof(small_buffer) - 1);
    EXPECT_EQ(std::string(small_buffer, read_size), test_data.substr(0, read_size));

    clog_file_close(&file);
    std::remove(path.c_str());
}

TEST_F(CLogFileSystemComprehensiveTest, ReadFileEdgeCases)
{
    const std::string path = GetTempFilePath();
    WriteTestDataToFile(path, "test");

    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_READ, 0644);
    ASSERT_NE(file, nullptr);

    char buffer[10];
    size_t read_size = 0;
    clog_res_e result = clog_file_read(file, buffer, 0, &read_size);
    EXPECT_EQ(result, CLOG_INVALID_PARAM);

    result = clog_file_read(file, nullptr, 10, &read_size);
    EXPECT_EQ(result, CLOG_INVALID_PARAM);

    clog_file_close(&file);
    std::remove(path.c_str());
}

TEST_F(CLogFileSystemComprehensiveTest, GetCurrentWorkingDirectory)
{
    char cwd_buffer[1024] = {0};
    clog_res_e result = clog_cwd(cwd_buffer, sizeof(cwd_buffer));
    EXPECT_EQ(result, CLOG_SUCCESS);
    EXPECT_GT(strlen(cwd_buffer), 0);

    result = clog_cwd(nullptr, 1024);
    EXPECT_EQ(result, CLOG_INVALID_PARAM);

    char dummy_buffer[10];
    result = clog_cwd(dummy_buffer, 0);
    EXPECT_EQ(result, CLOG_INVALID_PARAM);
}

TEST_F(CLogFileSystemComprehensiveTest, NormalizePath)
{
    char normalized_path[1024] = {};

#ifdef CLOG_PLATFORM_WINDOWS
    clog_res_e result = clog_normalize(R"(C:\Users\test\..\Documents)", normalized_path, sizeof(normalized_path));
    EXPECT_EQ(result, CLOG_SUCCESS);
    EXPECT_GT(strlen(normalized_path), 0);
#else
    clog_res_e result = clog_normalize("/home/user/../test/./subdir", normalized_path, sizeof(normalized_path));
    EXPECT_EQ(result, CLOG_SUCCESS);
    EXPECT_STREQ(normalized_path, "/home/test/subdir");
#endif

    result = clog_normalize(nullptr, normalized_path, sizeof(normalized_path));
    EXPECT_EQ(result, CLOG_INVALID_PARAM);

    result = clog_normalize("/test/path", nullptr, sizeof(normalized_path));
    EXPECT_EQ(result, CLOG_INVALID_PARAM);

    char small_buffer[2] = {};
    result = clog_normalize("/test", small_buffer, sizeof(small_buffer));
    EXPECT_NE(result, CLOG_SUCCESS);
}

TEST_F(CLogFileSystemComprehensiveTest, CreateDirectory)
{
    const std::string dir_path = GetTempDirPath();
    std::remove(dir_path.c_str());

    clog_res_e result = clog_dir_create(dir_path.c_str(), 0755);
    EXPECT_EQ(result, CLOG_SUCCESS);
    EXPECT_EQ(ACCESS(dir_path.c_str(), 0), 0);

    result = clog_dir_create(dir_path.c_str(), 0755);
    EXPECT_EQ(result, CLOG_ALREADY_EXISTED);

#ifdef CLOG_PLATFORM_WINDOWS
    _rmdir(dir_path.c_str());
#else
    rmdir(dir_path.c_str());
#endif
}

TEST_F(CLogFileSystemComprehensiveTest, CreateNestedDirectories)
{
    const std::string base_dir = GetTempDirPath();
    const std::string nested_dir = base_dir + "/level1/level2/level3";

    const clog_res_e result = clog_dir_create(nested_dir.c_str(), 0755);
    EXPECT_EQ(result, CLOG_SUCCESS);

    EXPECT_EQ(ACCESS(nested_dir.c_str(), 0), 0);

#ifdef CLOG_PLATFORM_WINDOWS
    _rmdir((base_dir + "/level1/level2/level3").c_str());
    _rmdir((base_dir + "/level1/level2").c_str());
    _rmdir((base_dir + "/level1").c_str());
    _rmdir(base_dir.c_str());
#else
    rmdir((base_dir + "/level1/level2/level3").c_str());
    rmdir((base_dir + "/level1/level2").c_str());
    rmdir((base_dir + "/level1").c_str());
    rmdir(base_dir.c_str());
#endif
}

TEST_F(CLogFileSystemComprehensiveTest, LargeDataOperations)
{
    const std::string path = GetTempFilePath();
    constexpr size_t large_data_size = 1024 * 1024;
    const std::string large_data(large_data_size, 'A');

    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_CREATE, 0644);
    ASSERT_NE(file, nullptr);

    clog_res_e result = clog_file_write(file, large_data.c_str(), large_data.length());
    EXPECT_EQ(result, CLOG_SUCCESS);
    clog_file_close(&file);

    file = clog_file_open(path.c_str(), CLOG_FILE_READ, 0644);
    ASSERT_NE(file, nullptr);

    std::string read_data(large_data_size, '\0');
    size_t read_size = 0;
    result = clog_file_read(file, &read_data[0], large_data_size, &read_size);
    EXPECT_EQ(result, CLOG_SUCCESS);
    EXPECT_EQ(read_size, large_data_size);
    EXPECT_EQ(read_data, large_data);

    clog_file_close(&file);
    std::remove(path.c_str());
}
