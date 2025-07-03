#include "router/router.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <memory>

using namespace flc;

class DummyHandler
{
public:
    void operator()()
    {
        // 空实现
    }
};

class HttpRouterPerformanceTest : public ::testing::Test
{
protected:
    void SetUp() override { router_ = std::make_unique<router<DummyHandler>>(); }

    void TearDown() override { router_->clear_all_routes(); }

    std::unique_ptr<router<DummyHandler>> router_;
};

TEST_F(HttpRouterPerformanceTest, BasicRouteLookup)
{
    router_->add_route(HttpMethod::GET, "/test", DummyHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result = router_->find_route(HttpMethod::GET, "/test", params, query_params);

    EXPECT_TRUE(result.has_value());
}

TEST_F(HttpRouterPerformanceTest, ParameterizedRouteLookup)
{
    router_->add_route(HttpMethod::GET, "/users/:id", DummyHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result = router_->find_route(HttpMethod::GET, "/users/123", params, query_params);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params["id"], "123");
}

TEST_F(HttpRouterPerformanceTest, WildcardRouteLookup)
{
    router_->add_route(HttpMethod::GET, "/static/*", DummyHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result =
        router_->find_route(HttpMethod::GET, "/static/css/style.css", params, query_params);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params["*"], "css/style.css");
}

TEST_F(HttpRouterPerformanceTest, LargeRouteSetPerformance)
{
    // 添加大量路由
    for (int i = 0; i < 1000; ++i) {
        std::string path = "/api/v1/users/" + std::to_string(i);
        router_->add_route(HttpMethod::GET, path, DummyHandler{});
    }

    // 性能测试
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        std::string path = "/api/v1/users/" + std::to_string(i % 1000);
        std::map<std::string, std::string> params, query_params;
        auto result = router_->find_route(HttpMethod::GET, path, params, query_params);
        EXPECT_TRUE(result.has_value());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 确保性能在合理范围内（10万次查找应该在几秒内完成）
    EXPECT_LT(duration.count(), 5000000); // 5秒内完成
}

TEST_F(HttpRouterPerformanceTest, PathSplittingPerformance)
{
    std::vector<std::string> segments;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100000; ++i) {
        router_->split_path_optimized("/api/v1/users/123/profile/settings", segments);
        EXPECT_EQ(segments.size(), 6);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 路径分割应该在1秒内完成100万次
    EXPECT_LT(duration.count(), 1000000);
}

TEST_F(HttpRouterPerformanceTest, UrlDecodingPerformance)
{
    std::string encoded = "Hello%20World%21%40%23%24%25%5E%26%2A%28%29";

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100000; ++i) {
        std::string test_str = encoded;
        router_->url_decode_safe(test_str);
        EXPECT_FALSE(test_str.empty());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // URL解码应该在1秒内完成100万次
    EXPECT_LT(duration.count(), 1000000);
}