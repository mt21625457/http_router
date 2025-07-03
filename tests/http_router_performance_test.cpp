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

TEST_F(HttpRouterPerformanceTest, QueryParameterParsing)
{
    router_->add_route(HttpMethod::GET, "/search", DummyHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result = router_->find_route(HttpMethod::GET, "/search?q=test&page=1&sort=name", params,
                                      query_params);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(query_params["q"], "test");
    EXPECT_EQ(query_params["page"], "1");
    EXPECT_EQ(query_params["sort"], "name");
}

TEST_F(HttpRouterPerformanceTest, MultipleHttpMethods)
{
    router_->add_route(HttpMethod::GET, "/users", DummyHandler{});
    router_->add_route(HttpMethod::POST, "/users", DummyHandler{});
    router_->add_route(HttpMethod::PUT, "/users", DummyHandler{});
    router_->add_route(HttpMethod::DELETE, "/users", DummyHandler{});

    std::map<std::string, std::string> params, query_params;

    // 测试GET方法
    auto result1 = router_->find_route(HttpMethod::GET, "/users", params, query_params);
    EXPECT_TRUE(result1.has_value());

    // 测试POST方法
    auto result2 = router_->find_route(HttpMethod::POST, "/users", params, query_params);
    EXPECT_TRUE(result2.has_value());

    // 测试PUT方法
    auto result3 = router_->find_route(HttpMethod::PUT, "/users", params, query_params);
    EXPECT_TRUE(result3.has_value());

    // 测试DELETE方法
    auto result4 = router_->find_route(HttpMethod::DELETE, "/users", params, query_params);
    EXPECT_TRUE(result4.has_value());

    // 测试不匹配的方法
    auto result5 = router_->find_route(HttpMethod::PATCH, "/users", params, query_params);
    EXPECT_FALSE(result5.has_value());
}

TEST_F(HttpRouterPerformanceTest, ComplexRoutePatterns)
{
    router_->add_route(HttpMethod::GET, "/api/:version/users/:userId/posts/:postId",
                       DummyHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result =
        router_->find_route(HttpMethod::GET, "/api/v1/users/123/posts/456", params, query_params);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params["version"], "v1");
    EXPECT_EQ(params["userId"], "123");
    EXPECT_EQ(params["postId"], "456");
}

TEST_F(HttpRouterPerformanceTest, StressTest)
{
    // 添加大量不同类型的路由
    for (int i = 0; i < 500; ++i) {
        std::string static_path = "/static" + std::to_string(i);
        router_->add_route(HttpMethod::GET, static_path, DummyHandler{});

        std::string param_path = "/api/v" + std::to_string(i % 3) + "/users/:id";
        router_->add_route(HttpMethod::GET, param_path, DummyHandler{});

        std::string wildcard_path = "/files/:type/*";
        router_->add_route(HttpMethod::GET, wildcard_path, DummyHandler{});
    }

    // 进行压力测试
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 5000; ++i) {
        // 测试静态路由
        std::string static_path = "/static" + std::to_string(i % 500);
        std::map<std::string, std::string> params1, query_params1;
        auto result1 = router_->find_route(HttpMethod::GET, static_path, params1, query_params1);
        EXPECT_TRUE(result1.has_value());

        // 测试参数化路由
        std::string param_path = "/api/v" + std::to_string((i % 3)) + "/users/" + std::to_string(i);
        std::map<std::string, std::string> params2, query_params2;
        auto result2 = router_->find_route(HttpMethod::GET, param_path, params2, query_params2);
        EXPECT_TRUE(result2.has_value());

        // 测试通配符路由
        std::string wildcard_path = "/files/image/path/to/file" + std::to_string(i) + ".jpg";
        std::map<std::string, std::string> params3, query_params3;
        auto result3 = router_->find_route(HttpMethod::GET, wildcard_path, params3, query_params3);
        EXPECT_TRUE(result3.has_value());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 压力测试应该在合理时间内完成
    EXPECT_LT(duration.count(), 10000000); // 10秒内完成
}