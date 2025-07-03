/**
 * @file http_router_test.cpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief Unit tests for the HTTP router with comprehensive route matching and
 * parameter extraction 包含全面路由匹配和参数提取的HTTP路由器单元测试
 * @version 0.1
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 */

#include "../include/router/router.hpp"
#include "../include/router/router_group.hpp"
#include "../include/router/router_impl.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace flc;

class DummyHandler
{
public:
    DummyHandler() = default;
    ~DummyHandler() = default;

    // Copy constructor and assignment (following Google style)
    DummyHandler(const DummyHandler &) = default;
    DummyHandler &operator=(const DummyHandler &) = default;

    // Move constructor and assignment
    DummyHandler(DummyHandler &&) = default;
    DummyHandler &operator=(DummyHandler &&) = default;

    explicit DummyHandler(int id) : id_(id) {}

    // 仿函数调用操作符（满足CallableHandler约束）
    void operator()() const {}

    // 仿函数调用操作符（满足CallableHandler约束）
    int id() const { return id_; }

private:
    int id_ = 0;
};

class HttpRouterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        router_ = std::make_unique<router<DummyHandler>>();
        params_.clear();
        query_params_.clear();
    }

    std::unique_ptr<router<DummyHandler>> router_;
    std::map<std::string, std::string> params_;
    std::map<std::string, std::string> query_params_;
};

TEST_F(HttpRouterTest, AddAndFindRoute)
{
    // Create test handlers
    DummyHandler handler1(1);
    DummyHandler handler2(2);
    DummyHandler handler3(3);

    // Test adding routes
    router_->add_route(HttpMethod::GET, "/test1", std::move(handler1));
    router_->add_route(HttpMethod::GET, "/test2", std::move(handler2));
    router_->add_route(HttpMethod::GET, "/test/nested/path", std::move(handler3));

    // Test finding existing routes
    auto result1 = router_->find_route(HttpMethod::GET, "/test1", params_, query_params_);
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value().get().id(), 1);

    auto result2 = router_->find_route(HttpMethod::GET, "/test2", params_, query_params_);
    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().get().id(), 2);

    auto result3 =
        router_->find_route(HttpMethod::GET, "/test/nested/path", params_, query_params_);
    EXPECT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value().get().id(), 3);

    // Test finding non-existent route
    auto result4 = router_->find_route(HttpMethod::GET, "/nonexistent", params_, query_params_);
    EXPECT_FALSE(result4.has_value());
}

TEST_F(HttpRouterTest, ParameterExtraction)
{
    DummyHandler handler(1);

    // Add a route with a parameter
    router_->add_route(HttpMethod::GET, "/users/:id", std::move(handler));

    // Test parameter extraction
    auto result = router_->find_route(HttpMethod::GET, "/users/123", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);
    EXPECT_EQ(params_["id"], "123");
}

TEST_F(HttpRouterTest, MultipleParameters)
{
    DummyHandler handler(1);

    // Add a route with multiple parameters
    router_->add_route(HttpMethod::GET, "/users/:userId/posts/:postId", std::move(handler));

    // Test multiple parameter extraction
    auto result =
        router_->find_route(HttpMethod::GET, "/users/123/posts/456", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);
    EXPECT_EQ(params_["userId"], "123");
    EXPECT_EQ(params_["postId"], "456");
}

TEST_F(HttpRouterTest, WildcardWithParameters)
{
    DummyHandler handler(1);

    // Add a route with wildcard and parameters
    router_->add_route(HttpMethod::GET, "/files/:path/*", std::move(handler));

    // Test wildcard with parameter extraction
    auto result =
        router_->find_route(HttpMethod::GET, "/files/documents/report.pdf", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);
    EXPECT_EQ(params_["path"], "documents");
    EXPECT_EQ(params_["*"], "report.pdf");
}

TEST_F(HttpRouterTest, QueryParameters)
{
    DummyHandler handler(1);

    // Add a static route
    router_->add_route(HttpMethod::GET, "/search", std::move(handler));

    // Test basic query parameter extraction
    auto result =
        router_->find_route(HttpMethod::GET, "/search?q=test&page=2", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);
    EXPECT_EQ(query_params_["q"], "test");
    EXPECT_EQ(query_params_["page"], "2");

    // Clear params for next test
    params_.clear();
    query_params_.clear();

    // Test URL-encoded query parameters
    auto result2 = router_->find_route(
        HttpMethod::GET, "/search?q=hello+world&filter=category%3Dbooks", params_, query_params_);
    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().get().id(), 1);
    EXPECT_EQ(query_params_["q"], "hello world");
    EXPECT_EQ(query_params_["filter"], "category=books");
}

TEST_F(HttpRouterTest, PathAndQueryParameters)
{
    DummyHandler handler(1);

    // Add a route with path parameters
    router_->add_route(HttpMethod::GET, "/users/:userId/posts/:postId", std::move(handler));

    // Test combination of path and query parameters
    auto result = router_->find_route(HttpMethod::GET, "/users/123/posts/456?sort=date&order=desc",
                                      params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);

    // Check path parameters
    EXPECT_EQ(params_["userId"], "123");
    EXPECT_EQ(params_["postId"], "456");

    // Check query parameters
    EXPECT_EQ(query_params_["sort"], "date");
    EXPECT_EQ(query_params_["order"], "desc");
}

TEST_F(HttpRouterTest, QueryParametersWithoutValue)
{
    DummyHandler handler(1);

    // Add a static route
    router_->add_route(HttpMethod::GET, "/options", std::move(handler));

    // Test query parameters without values
    auto result =
        router_->find_route(HttpMethod::GET, "/options?debug&verbose", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);
    EXPECT_EQ(query_params_["debug"], "");
    EXPECT_EQ(query_params_["verbose"], "");

    // Test mixed query parameters with and without values
    params_.clear();
    query_params_.clear();
    auto result2 =
        router_->find_route(HttpMethod::GET, "/options?debug&level=info", params_, query_params_);
    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().get().id(), 1);
    EXPECT_EQ(query_params_["debug"], "");
    EXPECT_EQ(query_params_["level"], "info");
}

// Test hybrid routing strategy - hash map for short paths, trie for long paths
TEST_F(HttpRouterTest, HybridRoutingStrategy)
{
    // Create handlers with IDs for differentiation
    DummyHandler short_handler(1); // Hash table storage
    DummyHandler long_handler(2);  // Trie storage
    DummyHandler param_handler(3); // Parameterized routes

    // Add various types of routes
    router_->add_route(HttpMethod::GET, "/api", std::move(short_handler)); // Short -> hash
    router_->add_route(HttpMethod::GET, "/api/users/profiles/settings/notifications",
                       std::move(long_handler)); // Long -> trie
    router_->add_route(HttpMethod::GET, "/products/:category/:id",
                       std::move(param_handler)); // Parameterized

    // Test short path lookup (hash table)
    auto result1 = router_->find_route(HttpMethod::GET, "/api", params_, query_params_);
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value().get().id(), 1);

    // Test long path lookup (trie)
    params_.clear();
    query_params_.clear();
    auto result2 = router_->find_route(
        HttpMethod::GET, "/api/users/profiles/settings/notifications", params_, query_params_);
    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().get().id(), 2);

    // Test parameterized route lookup
    params_.clear();
    query_params_.clear();
    auto result3 =
        router_->find_route(HttpMethod::GET, "/products/electronics/12345", params_, query_params_);
    EXPECT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value().get().id(), 3);
    EXPECT_EQ(params_["category"], "electronics");
    EXPECT_EQ(params_["id"], "12345");
}

TEST_F(HttpRouterTest, ComplexRoutingScenario)
{
    // Add multiple routes with various characteristics
    std::vector<DummyHandler> handlers;
    for (int i = 0; i < 50; ++i) {
        handlers.push_back(DummyHandler(i));
    }

    // Short path routes
    router_->add_route(HttpMethod::GET, "/", std::move(handlers[0]));
    router_->add_route(HttpMethod::GET, "/home", std::move(handlers[1]));
    router_->add_route(HttpMethod::GET, "/about", std::move(handlers[2]));
    router_->add_route(HttpMethod::GET, "/contact", std::move(handlers[3]));
    router_->add_route(HttpMethod::POST, "/login", std::move(handlers[4]));
    router_->add_route(HttpMethod::POST, "/signup", std::move(handlers[5]));

    // Long path routes (trie)
    router_->add_route(HttpMethod::GET, "/api/v1/users/profiles/settings", std::move(handlers[10]));
    router_->add_route(HttpMethod::GET, "/api/v1/users/profiles/photos", std::move(handlers[11]));
    router_->add_route(HttpMethod::GET, "/api/v1/users/profiles/friends", std::move(handlers[12]));
    router_->add_route(HttpMethod::GET, "/api/v1/posts/recent/comments", std::move(handlers[13]));
    router_->add_route(HttpMethod::GET, "/api/v1/posts/trending/today", std::move(handlers[14]));

    // Parameterized routes
    router_->add_route(HttpMethod::GET, "/users/:userId", std::move(handlers[20]));
    router_->add_route(HttpMethod::PUT, "/users/:userId/profile", std::move(handlers[21]));
    router_->add_route(HttpMethod::DELETE, "/users/:userId/posts/:postId", std::move(handlers[22]));
    router_->add_route(HttpMethod::GET, "/products/:category/:productId/reviews",
                       std::move(handlers[23]));

    // Wildcard routes
    router_->add_route(HttpMethod::GET, "/static/*", std::move(handlers[30]));
    router_->add_route(HttpMethod::GET, "/files/:type/*", std::move(handlers[31]));

    // Test wildcard
    auto result1 = router_->find_route(HttpMethod::GET, "/static/123", params_, query_params_);
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value().get().id(), 30);
    EXPECT_EQ(params_["*"], "123");

    // Test short path
    auto result2 = router_->find_route(HttpMethod::GET, "/about", params_, query_params_);
    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().get().id(), 2);

    // Test long path
    params_.clear();
    query_params_.clear();
    auto result3 = router_->find_route(HttpMethod::GET, "/api/v1/posts/trending/today", params_,
                                       query_params_);
    EXPECT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value().get().id(), 14);

    // Test parameterized route
    params_.clear();
    query_params_.clear();
    auto result4 =
        router_->find_route(HttpMethod::DELETE, "/users/42/posts/123", params_, query_params_);
    EXPECT_TRUE(result4.has_value());
    EXPECT_EQ(result4.value().get().id(), 22);
    EXPECT_EQ(params_["userId"], "42");
    EXPECT_EQ(params_["postId"], "123");

    // Test wildcard route with parameters
    params_.clear();
    query_params_.clear();
    auto result5 = router_->find_route(HttpMethod::GET, "/files/documents/reports/annual/2023.pdf",
                                       params_, query_params_);
    EXPECT_TRUE(result5.has_value());
    EXPECT_EQ(result5.value().get().id(), 31);
    EXPECT_EQ(params_["type"], "documents");
    EXPECT_EQ(params_["*"], "reports/annual/2023.pdf");

    // Test mixed path and query parameters
    params_.clear();
    query_params_.clear();
    auto result6 = router_->find_route(HttpMethod::GET,
                                       "/products/electronics/12345/reviews?sort=newest&page=2",
                                       params_, query_params_);
    EXPECT_TRUE(result6.has_value());
    EXPECT_EQ(result6.value().get().id(), 23);
    EXPECT_EQ(params_["category"], "electronics");
    EXPECT_EQ(params_["productId"], "12345");
    EXPECT_EQ(query_params_["sort"], "newest");
    EXPECT_EQ(query_params_["page"], "2");
}

TEST_F(HttpRouterTest, RouteConflictResolution)
{
    // Create handlers
    DummyHandler handler1(1);
    DummyHandler handler2(2);
    DummyHandler handler3(3);

    // Add potentially conflicting routes
    router_->add_route(HttpMethod::GET, "/api/users", std::move(handler1));
    router_->add_route(HttpMethod::GET, "/api/:resource", std::move(handler2));
    router_->add_route(HttpMethod::GET, "/api/users/:id", std::move(handler3));

    // Test static route takes priority over parameterized route
    auto result1 = router_->find_route(HttpMethod::GET, "/api/users", params_, query_params_);
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value().get().id(), 1);

    // Test parameterized route matches other resources
    params_.clear();
    EXPECT_TRUE(
        router_->find_route(HttpMethod::GET, "/api/products", params_, query_params_).has_value());
    EXPECT_EQ(params_["resource"], "products");

    // Test more specific parameterized route
    params_.clear();
    EXPECT_TRUE(
        router_->find_route(HttpMethod::GET, "/api/users/42", params_, query_params_).has_value());
    EXPECT_EQ(params_["id"], "42");
}

TEST_F(HttpRouterTest, PerformanceBenchmark)
{
    // Add many routes for performance testing
    constexpr int kNumRoutes = 1000;
    std::vector<DummyHandler> handlers;

    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.push_back(DummyHandler(i));
        HttpMethod method = static_cast<HttpMethod>(i % 5);
        if (method == HttpMethod::UNKNOWN) {
            method = HttpMethod::GET;
        }

        // Add various types of routes
        if (i % 5 == 0) {
            // Short paths
            router_->add_route(method, "/short" + std::to_string(i), std::move(handlers[i]));
        } else if (i % 5 == 1) {
            // Long paths
            router_->add_route(method, "/api/v1/users/profiles/settings/" + std::to_string(i),
                               std::move(handlers[i]));
        } else if (i % 5 == 2) {
            // Parameterized routes
            router_->add_route(method, "/users/" + std::to_string(i) + "/:param",
                               std::move(handlers[i]));
        } else if (i % 5 == 3) {
            // Multiple parameter routes
            router_->add_route(method, "/products/:category/" + std::to_string(i) + "/:id",
                               std::move(handlers[i]));
        } else {
            // Wildcard routes
            router_->add_route(method, "/files/" + std::to_string(i) + "/*",
                               std::move(handlers[i]));
        }
    }

    // Execute some route matching to verify performance
    auto result = router_->find_route(HttpMethod::GET, "/short100", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 100);

    params_.clear();
    EXPECT_TRUE(router_
                    ->find_route(HttpMethod::POST, "/api/v1/users/profiles/settings/101", params_,
                                 query_params_)
                    .has_value());
    EXPECT_EQ(params_["id"], "101");
}

TEST_F(HttpRouterTest, RouteCaching)
{
    // Create route handlers
    DummyHandler handler1(1);
    DummyHandler handler2(2);
    DummyHandler handler3(3);

    // Add various types of routes
    router_->add_route(HttpMethod::GET, "/api/users", std::move(handler1));
    router_->add_route(HttpMethod::POST, "/api/posts/:postId", std::move(handler2));
    router_->add_route(HttpMethod::GET, "/files/documents/*", std::move(handler3));

    // First lookup - should require regular route matching
    auto start_time = std::chrono::high_resolution_clock::now();
    auto result1 = router_->find_route(HttpMethod::POST, "/api/posts/123", params_, query_params_);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto first_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value().get().id(), 2);
    EXPECT_EQ(params_["postId"], "123");

    // Clear parameters
    params_.clear();

    // Second lookup of same path - should hit cache
    start_time = std::chrono::high_resolution_clock::now();
    auto result2 = router_->find_route(HttpMethod::POST, "/api/posts/123", params_, query_params_);
    end_time = std::chrono::high_resolution_clock::now();
    auto second_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().get().id(), 2);
    EXPECT_EQ(params_["postId"], "123");

    std::cout << "First lookup: " << first_duration << " ns" << std::endl;
    std::cout << "Second lookup (cached): " << second_duration << " ns" << std::endl;

    // Test wildcard route caching
    params_.clear();
    query_params_.clear();
    auto result3 =
        router_->find_route(HttpMethod::GET, "/files/documents/report.pdf", params_, query_params_);
    EXPECT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value().get().id(), 3);
    EXPECT_EQ(params_["*"], "report.pdf");
}

TEST_F(HttpRouterTest, AddAndFindWithDifferentMethods)
{
    DummyHandler handler1(1);
    DummyHandler handler2(2);
    DummyHandler handler3(3);
    DummyHandler handler4(4);

    // Add same path with different methods
    router_->add_route(HttpMethod::GET, "/api/resource", std::move(handler1));
    router_->add_route(HttpMethod::POST, "/api/resource", std::move(handler2));
    router_->add_route(HttpMethod::PUT, "/api/resource", std::move(handler3));
    router_->add_route(HttpMethod::DELETE, "/api/resource", std::move(handler4));

    // Test each method finds correct handler
    auto result1 = router_->find_route(HttpMethod::GET, "/api/resource", params_, query_params_);
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value().get().id(), 1);

    auto result2 = router_->find_route(HttpMethod::POST, "/api/resource", params_, query_params_);
    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().get().id(), 2);

    auto result3 = router_->find_route(HttpMethod::PUT, "/api/resource", params_, query_params_);
    EXPECT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value().get().id(), 3);

    auto result4 = router_->find_route(HttpMethod::DELETE, "/api/resource", params_, query_params_);
    EXPECT_TRUE(result4.has_value());
    EXPECT_EQ(result4.value().get().id(), 4);

    // Test non-registered method
    EXPECT_FALSE(router_->find_route(HttpMethod::PATCH, "/api/resource", params_, query_params_)
                     .has_value());
}

TEST_F(HttpRouterTest, MethodSpecificCache)
{
    DummyHandler handler1(1);
    DummyHandler handler2(2);

    router_->add_route(HttpMethod::GET, "/api/:id", std::move(handler1));
    router_->add_route(HttpMethod::POST, "/api/:id", std::move(handler2));

    // First access to warm up cache
    auto result1 = router_->find_route(HttpMethod::GET, "/api/123", params_, query_params_);
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value().get().id(), 1);
    EXPECT_EQ(params_["id"], "123");

    params_.clear();
    auto result2 = router_->find_route(HttpMethod::POST, "/api/123", params_, query_params_);
    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().get().id(), 2);
    EXPECT_EQ(params_["id"], "123");

    // Test cached access for each method
    params_.clear();
    auto result3 = router_->find_route(HttpMethod::GET, "/api/123", params_, query_params_);
    EXPECT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value().get().id(), 1);
    EXPECT_EQ(params_["id"], "123");

    params_.clear();
    auto result4 = router_->find_route(HttpMethod::POST, "/api/123", params_, query_params_);
    EXPECT_TRUE(result4.has_value());
    EXPECT_EQ(result4.value().get().id(), 2);
    EXPECT_EQ(params_["id"], "123");
}

TEST_F(HttpRouterTest, AddRouteWithUnknownMethod)
{
    DummyHandler handler(1);

    // Adding route with UNKNOWN method should still work
    router_->add_route(HttpMethod::UNKNOWN, "/unknown", std::move(handler));

    // Should be findable
    auto result = router_->find_route(HttpMethod::UNKNOWN, "/unknown", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);
}
