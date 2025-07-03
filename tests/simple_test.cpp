#include "router/router.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace flc;

class TestHandler
{
private:
    int id_;

public:
    explicit TestHandler(int id) : id_(id) {}
    // 仿函数调用操作符（满足CallableHandler约束）
    void operator()() const tests/simple_test.cpp

    int id() const { return id_; }
    void handle() const {}
};

class SimpleRouterTest : public ::testing::Test
{
protected:
    std::unique_ptr<router<TestHandler>> router_;

    void SetUp() override { router_ = std::make_unique<router<TestHandler>>(); }

    void TearDown() override
    {
        try {
            if (router_) {
                router_->clear_cache();
                router_.reset();
            }
        } catch (...) {
            // 忽略析构过程中的异常
        }
    }
};

TEST_F(SimpleRouterTest, BasicFunctionality)
{
    std::vector<std::string> segments;

    // 测试基本路径分割
    router_->split_path_optimized("/api/users", segments);
    ASSERT_EQ(segments.size(), 2);
    EXPECT_EQ(segments[0], "api");
    EXPECT_EQ(segments[1], "users");

    // 测试URL解码
    std::string test_str = "hello%20world";
    router_->url_decode_safe(test_str);
    EXPECT_EQ(test_str, "hello world");

    // 测试十六进制转换
    int value;
    EXPECT_TRUE(router_->hex_to_int_safe('A', value));
    EXPECT_EQ(value, 10);
}

TEST_F(SimpleRouterTest, CacheKeyBuilder)
{
    auto &builder = router_->get_thread_local_cache_key_builder();
    const std::string &key = builder.build(HttpMethod::GET, "/api/users");
    EXPECT_EQ(key, "GET:/api/users");
}