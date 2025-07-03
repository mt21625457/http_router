/**
 * @file router_simple_test.cpp
 * @brief 简单的路由器测试，验证unique_ptr接口设计
 * @version 1.0
 * @date 2025-03-26
 */

#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>

#include "router/router.hpp"

using namespace flc;

/**
 * @brief 测试用的仿函数处理器类
 */
class SimpleHandler
{
public:
    void operator()()
    {
        // 空实现
    }
};

/**
 * @class SimpleRouterTest
 * @brief 简单的路由器测试套件
 */
class SimpleRouterTest : public ::testing::Test
{
protected:
    void SetUp() override { 
        router_.reset(new router<SimpleHandler>()); 
        params_.clear();
        query_params_.clear();
    }

    void TearDown() override
    {
        // 移除缓存清理
    }

    std::unique_ptr<router<SimpleHandler>> router_;
    std::map<std::string, std::string> params_;
    std::map<std::string, std::string> query_params_;
};

/**
 * @brief 测试基本的路由添加和查找功能
 */
TEST_F(SimpleRouterTest, BasicRouteTest)
{
    router_->add_route(HttpMethod::GET, "/test", SimpleHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result = router_->find_route(HttpMethod::GET, "/test", params, query_params);

    EXPECT_TRUE(result.has_value());
}

/**
 * @brief 测试参数化路由
 */
TEST_F(SimpleRouterTest, ParameterizedRouteTest)
{
    router_->add_route(HttpMethod::GET, "/users/:id", SimpleHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result = router_->find_route(HttpMethod::GET, "/users/123", params, query_params);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params["id"], "123");
}

/**
 * @brief 测试通配符路由
 */
TEST_F(SimpleRouterTest, WildcardRouteTest)
{
    router_->add_route(HttpMethod::GET, "/static/*", SimpleHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result =
        router_->find_route(HttpMethod::GET, "/static/css/style.css", params, query_params);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params["*"], "css/style.css");
}

/**
 * @brief 测试查询参数解析
 */
TEST_F(SimpleRouterTest, QueryParameters)
{
    router_->add_route(HttpMethod::GET, "/search", SimpleHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result =
        router_->find_route(HttpMethod::GET, "/search?q=test&sort=date", params, query_params);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(query_params["q"], "test");
    EXPECT_EQ(query_params["sort"], "date");
}

/**
 * @brief 测试路由未找到的情况
 */
TEST_F(SimpleRouterTest, RouteNotFound)
{
    // 添加一个路由
    router_->add_route(HttpMethod::GET, "/api/users", SimpleHandler{});

    // 查找不存在的路由
    auto result = router_->find_route(HttpMethod::GET, "/api/posts", params_, query_params_);

    EXPECT_FALSE(result.has_value());
}

/**
 * @brief 测试不同HTTP方法
 */
TEST_F(SimpleRouterTest, MultipleHttpMethods)
{
    router_->add_route(HttpMethod::GET, "/users", SimpleHandler{});
    router_->add_route(HttpMethod::POST, "/users", SimpleHandler{});

    std::map<std::string, std::string> params, query_params;

    auto result1 = router_->find_route(HttpMethod::GET, "/users", params, query_params);
    EXPECT_TRUE(result1.has_value());

    auto result2 = router_->find_route(HttpMethod::POST, "/users", params, query_params);
    EXPECT_TRUE(result2.has_value());

    auto result3 = router_->find_route(HttpMethod::PUT, "/users", params, query_params);
    EXPECT_FALSE(result3.has_value());
}

/**
 * @brief 测试Lambda表达式支持
 */
TEST_F(SimpleRouterTest, LambdaExpressionSupport)
{
    // 测试简单lambda
    {
        auto simple_lambda = []() {
            // 简单的lambda处理器
        };

        router<decltype(simple_lambda)> lambda_router;
        lambda_router.add_route(HttpMethod::GET, "/lambda/simple", std::move(simple_lambda));
        
        std::map<std::string, std::string> local_params, local_query_params;
        auto result =
            lambda_router.find_route(HttpMethod::GET, "/lambda/simple", local_params, local_query_params);
        EXPECT_TRUE(result.has_value());
        if (result.has_value()) {
            result->get()(); // 调用lambda
        }
    }

    // 测试带值捕获的lambda
    {
        auto lambda_with_capture = [call_count = 0]() mutable -> int { return ++call_count; };

        router<decltype(lambda_with_capture)> capture_router;
        capture_router.add_route(HttpMethod::GET, "/lambda/capture",
                                 std::move(lambda_with_capture));

        std::map<std::string, std::string> local_params, local_query_params;
        auto result =
            capture_router.find_route(HttpMethod::GET, "/lambda/capture", local_params, local_query_params);
        EXPECT_TRUE(result.has_value());

        if (result.has_value()) {
            int count1 = result->get()();
            EXPECT_EQ(count1, 1);

            int count2 = result->get()();
            EXPECT_EQ(count2, 2);
        }
    }
}

/**
 * @brief 测试复杂路由
 */
TEST_F(SimpleRouterTest, ComplexRoutes)
{
    router_->add_route(HttpMethod::GET, "/api/posts", SimpleHandler{});
    router_->add_route(HttpMethod::POST, "/api/posts", SimpleHandler{});
    router_->add_route(HttpMethod::GET, "/api/posts/:id", SimpleHandler{});
    router_->add_route(HttpMethod::PUT, "/api/posts/:id", SimpleHandler{});
    router_->add_route(HttpMethod::DELETE, "/api/posts/:id", SimpleHandler{});

    std::map<std::string, std::string> params, query_params;

    // 测试静态路由
    auto result = router_->find_route(HttpMethod::GET, "/api/posts", params, query_params);
    EXPECT_TRUE(result.has_value());

    // 测试参数化路由
    result = router_->find_route(HttpMethod::GET, "/api/posts/123", params, query_params);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params["id"], "123");

    // 测试不同的HTTP方法
    result = router_->find_route(HttpMethod::POST, "/api/posts", params, query_params);
    EXPECT_TRUE(result.has_value());

    result = router_->find_route(HttpMethod::PUT, "/api/posts/456", params, query_params);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params["id"], "456");
}

/**
 * @brief 测试Lambda处理器
 */
TEST_F(SimpleRouterTest, LambdaHandlers)
{
    // 测试lambda处理器
    auto lambda_handler = []() { /* 空实现 */ };

    router<decltype(lambda_handler)> lambda_router;
    lambda_router.add_route(HttpMethod::GET, "/lambda/simple", std::move(lambda_handler));

    std::map<std::string, std::string> params, query_params;
    auto result = lambda_router.find_route(HttpMethod::GET, "/lambda/simple", params, query_params);
    EXPECT_TRUE(result.has_value());
}

/**
 * @brief 测试带捕获的lambda处理器
 */
TEST_F(SimpleRouterTest, CaptureLambdaHandlers)
{
    // 测试带捕获的lambda处理器
    int captured_value = 42;
    auto capture_handler = [captured_value]() {
        // 使用捕获的值
        (void)captured_value;
    };

    router<decltype(capture_handler)> capture_router;
    capture_router.add_route(HttpMethod::GET, "/lambda/capture", std::move(capture_handler));

    std::map<std::string, std::string> params, query_params;
    auto result =
        capture_router.find_route(HttpMethod::GET, "/lambda/capture", params, query_params);
    EXPECT_TRUE(result.has_value());
}