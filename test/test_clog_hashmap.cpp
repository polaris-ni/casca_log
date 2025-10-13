/**
 * @auther Polaris
 * @date  2025/10/8
 */
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include "utils/clog_hashmap.h"

class ClogHashMapTest : public testing::Test
{
protected:
    clog_hashmap_t* map = nullptr;

    void SetUp() override
    {
        map = clog_hashmap_create(0, 0, clog_hashmap_string_dup, clog_hashmap_string_free, clog_hashmap_string_dup,
                                  clog_hashmap_string_free, clog_hashmap_string_cmp, clog_hashmap_string_size, 0);
        ASSERT_NE(nullptr, map);
    }

    void TearDown() override
    {
        clog_hashmap_destroy(&map);
    }
};

TEST_F(ClogHashMapTest, CreateAndDestroy)
{
    /* tested in SetUp/TearDown */
}

TEST_F(ClogHashMapTest, PutAndGet)
{
    const auto key = "test_key";
    const auto value = "test_value";

    EXPECT_EQ(CLOG_SUCCESS, clog_hashmap_put(map, key, value));

    const auto retrieved = static_cast<const char*>(clog_hashmap_get(map, key));
    ASSERT_NE(nullptr, retrieved);
    EXPECT_STREQ(value, retrieved);
}

TEST_F(ClogHashMapTest, GetNonExistentKey)
{
    const auto key = "non_existent";

    const auto retrieved = static_cast<const char*>(clog_hashmap_get(map, key));
    EXPECT_EQ(nullptr, retrieved);
}

TEST_F(ClogHashMapTest, ReplaceValue)
{
    const auto key = "test_key";
    const auto value1 = "value1";
    const auto value2 = "value2";

    EXPECT_EQ(CLOG_SUCCESS, clog_hashmap_put(map, key, value1));
    EXPECT_EQ(CLOG_SUCCESS, clog_hashmap_put(map, key, value2));

    const auto retrieved = static_cast<const char*>(clog_hashmap_get(map, key));
    ASSERT_NE(nullptr, retrieved);
    EXPECT_STREQ(value2, retrieved);
}

TEST_F(ClogHashMapTest, RemoveKey)
{
    const auto key = "test_key";
    const auto value = "test_value";

    EXPECT_EQ(CLOG_SUCCESS, clog_hashmap_put(map, key, value));
    EXPECT_TRUE(clog_hashmap_remove(map, key));

    const auto retrieved = static_cast<const char*>(clog_hashmap_get(map, key));
    EXPECT_EQ(nullptr, retrieved);
}

TEST_F(ClogHashMapTest, RemoveNonExistentKey)
{
    const auto key = "non_existent";
    EXPECT_FALSE(clog_hashmap_remove(map, key));
}

TEST_F(ClogHashMapTest, ExistsCheck)
{
    const auto key = "test_key";
    const auto value = "test_value";

    EXPECT_FALSE(clog_hashmap_is_exists(map, key));
    EXPECT_EQ(CLOG_SUCCESS, clog_hashmap_put(map, key, value));
    EXPECT_TRUE(clog_hashmap_is_exists(map, key));
}

TEST_F(ClogHashMapTest, TakeValue)
{
    const auto key = "test_key";
    const auto value = "test_value";

    EXPECT_EQ(CLOG_SUCCESS, clog_hashmap_put(map, key, value));

    const auto taken = static_cast<char*>(clog_hashmap_take(map, key));
    ASSERT_NE(nullptr, taken);
    EXPECT_STREQ(value, taken);
    free(taken);

    EXPECT_FALSE(clog_hashmap_is_exists(map, key));
}

TEST_F(ClogHashMapTest, ClearMap)
{
    EXPECT_EQ(CLOG_SUCCESS, clog_hashmap_put(map, "key1", "value1"));
    EXPECT_EQ(CLOG_SUCCESS, clog_hashmap_put(map, "key2", "value2"));

    clog_hashmap_clear(map);

    EXPECT_EQ(0u, clog_hashmap_size(map));
    EXPECT_FALSE(clog_hashmap_is_exists(map, "key1"));
    EXPECT_FALSE(clog_hashmap_is_exists(map, "key2"));
}

TEST_F(ClogHashMapTest, ResizeAutomatically)
{
    for (int i = 0; i < 32; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string value = "value" + std::to_string(i);
        EXPECT_EQ(CLOG_SUCCESS, clog_hashmap_put(map, key.c_str(), value.c_str()));
    }

    for (int i = 0; i < 32; ++i) {
        std::string key = "key" + std::to_string(i);
        const auto retrieved = static_cast<const char*>(clog_hashmap_get(map, key.c_str()));
        ASSERT_NE(nullptr, retrieved);
        std::string expected = "value" + std::to_string(i);
        EXPECT_STREQ(expected.c_str(), retrieved);
    }
}

TEST_F(ClogHashMapTest, NullParametersHandling)
{
    EXPECT_EQ(CLOG_INVALID_PARAM, clog_hashmap_put(nullptr, "key", "value"));
    EXPECT_EQ(nullptr, clog_hashmap_get(nullptr, "key"));
    EXPECT_FALSE(clog_hashmap_remove(nullptr, "key"));
    EXPECT_FALSE(clog_hashmap_is_exists(nullptr, "key"));
    EXPECT_EQ(nullptr, clog_hashmap_take(nullptr, "key"));
}
