/**
 * @author Polaris
 * @date 2026/2/13
 */

#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include "clog_file_system.h"
#include "test_util.h"

class CLogFileSystemTest : public testing::Test {
protected:
    void SetUp() override {
        CLogTest::CLogMemLeakDetect::start(nullptr, nullptr);
    }

    void TearDown() override {
        CLogTest::CLogMemLeakDetect::end();
    }

    static std::string GetTempFilePath() {
        static int counter = 0;
#ifdef CLOG_PLATFORM_LINUX
        return "./temp_test_file_" + std::to_string(counter++) + ".txt";
#else
        return "temp_test_file_" + std::to_string(counter++) + ".txt";
#endif
    }
};

TEST_F(CLogFileSystemTest, OpenFileBasic) {
    const std::string path = GetTempFilePath();
    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_CREATE);
    EXPECT_NE(file, nullptr);

    clog_file_close(file);
    std::remove(path.c_str());
}

TEST_F(CLogFileSystemTest, OpenFileInvalidPath) {
    clog_file_t *file = clog_file_open(nullptr, CLOG_FILE_WRITE);
    EXPECT_EQ(file, nullptr);

    file = clog_file_open("", CLOG_FILE_WRITE);
    EXPECT_EQ(file, nullptr);
}

TEST_F(CLogFileSystemTest, OpenFileNoPermission) {
    const std::string path = "/root/test_file.txt"; // 假设没有权限访问此路径
    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_WRITE);
    EXPECT_EQ(file, nullptr);
}

TEST_F(CLogFileSystemTest, WriteToFile) {
    const std::string path = GetTempFilePath();
    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_CREATE);
    ASSERT_NE(file, nullptr);

    const auto data = "Hello, World!";
    const clog_res_e result = clog_file_write(file, data, strlen(data));
    EXPECT_EQ(result, CLOG_SUCCESS);

    clog_file_close(file);
    std::remove(path.c_str());
}

TEST_F(CLogFileSystemTest, WriteToFileWithoutWriteFlag) {
    const std::string path = GetTempFilePath();
    std::ofstream(path, std::ios::out | std::ios::trunc).close();
    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_READ);
    ASSERT_NE(file, nullptr);

    const auto data = "Hello, World!";
    const clog_res_e result = clog_file_write(file, data, strlen(data));
    EXPECT_EQ(result, CLOG_NOT_SUPPORTED);

    clog_file_close(file);
    std::remove(path.c_str());
}

TEST_F(CLogFileSystemTest, ReadFromFile) {
    const std::string path = GetTempFilePath();
    const auto data = "Hello, World!";

    clog_file_t *write_file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_CREATE);
    ASSERT_NE(write_file, nullptr);
    clog_file_write(write_file, data, strlen(data));
    clog_file_close(write_file);

    clog_file_t *read_file = clog_file_open(path.c_str(), CLOG_FILE_READ);
    ASSERT_NE(read_file, nullptr);

    char buffer[128] = {0};
    size_t read_size = 0;
    const clog_res_e result = clog_file_read(read_file, buffer, sizeof(buffer), &read_size);
    EXPECT_EQ(result, CLOG_SUCCESS);
    EXPECT_EQ(read_size, strlen(data));
    EXPECT_STREQ(buffer, data);

    clog_file_close(read_file);
    std::remove(path.c_str());
}

TEST_F(CLogFileSystemTest, ReadFromFileWithoutReadFlag) {
    const std::string path = GetTempFilePath();
    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_CREATE);
    ASSERT_NE(file, nullptr);

    char buffer[128] = {};
    size_t read_size = 0;
    const clog_res_e result = clog_file_read(file, buffer, sizeof(buffer), &read_size);
    EXPECT_EQ(result, CLOG_NOT_SUPPORTED);

    clog_file_close(file);
    std::remove(path.c_str());
}

TEST_F(CLogFileSystemTest, CloseFile) {
    const std::string path = GetTempFilePath();
    clog_file_t *file = clog_file_open(path.c_str(), CLOG_FILE_WRITE | CLOG_FILE_CREATE);
    ASSERT_NE(file, nullptr);

    clog_file_close(file);
    std::remove(path.c_str());
}
