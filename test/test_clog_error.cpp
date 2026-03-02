/**
 * @auther polaris
 * @date  2025/10/21
 */
#include <gtest/gtest.h>
#include "clog_error.h"
#include "test_util.h"

class CLogErrorTest : public testing::Test
{
protected:
    void SetUp() override
    {
        CLogTest::CLogMemLeakDetect::start(nullptr, nullptr);
        clog_err_setup(10, 256);
    }

    void TearDown() override
    {
        clog_err_cleanup();
        CLogTest::CLogMemLeakDetect::end();
    }
};

TEST_F(CLogErrorTest, Initialization)
{
    unsigned int num = 0;
    const char **errors = clog_err_get(&num);

    EXPECT_EQ(errors, nullptr);
    EXPECT_EQ(num, 0);
}

TEST_F(CLogErrorTest, PutSingleError)
{
    clog_err_put(__FILE__, __LINE__, "Test error message");

    unsigned int num = 0;
    const char **errors = clog_err_get(&num);

    ASSERT_NE(errors, nullptr);
    EXPECT_EQ(num, 1);
}

TEST_F(CLogErrorTest, PutMultipleErrors)
{
    for (int i = 0; i < 5; i++) {
        clog_err_put(__FILE__, __LINE__, "Test error message %d", i);
    }

    unsigned int num = 0;
    const char **errors = clog_err_get(&num);

    ASSERT_NE(errors, nullptr);
    EXPECT_EQ(num, 5);
}

TEST_F(CLogErrorTest, CircularOverwrite)
{
    for (int i = 0; i < 15; i++) {
        clog_err_put(__FILE__, __LINE__, "Test error message %d", i);
    }

    unsigned int num = 0;
    const char **errors = clog_err_get(&num);

    ASSERT_NE(errors, nullptr);
    EXPECT_EQ(num, 10);
}

TEST_F(CLogErrorTest, ClearErrors)
{
    clog_err_put(__FILE__, __LINE__, "Test error message");

    unsigned int num = 0;
    clog_err_get(&num);
    EXPECT_EQ(num, 1);

    clog_err_clear();

    num = 0;
    const char **errors = clog_err_get(&num);
    EXPECT_EQ(errors, nullptr);
    EXPECT_EQ(num, 0);
}

TEST_F(CLogErrorTest, PrintToBuffer)
{
    clog_err_put(__FILE__, __LINE__, "First error");
    clog_err_put(__FILE__, __LINE__, "Second error");

    char buffer[512] = {0};
    clog_err_print(buffer, sizeof(buffer), "\n");

    EXPECT_STRNE(buffer, "");
    EXPECT_NE(strstr(buffer, "First error"), nullptr);
    EXPECT_NE(strstr(buffer, "Second error"), nullptr);
}

TEST(CLogErrorBoundaryTest, ZeroSizeSetup)
{
    clog_err_setup(0, 10);
    clog_err_setup(10, 0);

    unsigned int num = 0;
    const char **errors = clog_err_get(&num);

    EXPECT_EQ(errors, nullptr);
    EXPECT_EQ(num, 0);
}
