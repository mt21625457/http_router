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
    void operator()() const tests/http_router_test.cpp

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
        found_handler_ = nullptr;
    }

    std::unique_ptr<router<DummyHandler>> router_;
    std::shared_ptr<DummyHandler> found_handler_;
    std::map<std::string, std::string> params_;
    std::map<std::string, std::string> query_params_;
};

TEST_F(HttpRouterTest, AddAndFindRoute)
{
    // Create test handlers
    auto handler1 = std::make_shared<DummyHandler>(1);
    auto handler2 = std::make_shared<DummyHandler>(2);
    auto handler3 = std::make_shared<DummyHandler>(3);

    // Test adding routes
    router_->add_route(HttpMethod::GET, "/test1", handler1);
    router_->add_route(HttpMethod::GET, "/test2", handler2);
    router_->add_route(HttpMethod::GET, "/test/nested/path", handler3);

    // Test finding existing routes
    EXPECT_EQ(
        router_->find_route(HttpMethod::GET, "/test1", found_handler_, params_, query_params_), 0);
    EXPECT_EQ(found_handler_, handler1);

    EXPECT_EQ(
        router_->find_route(HttpMethod::GET, "/test2", found_handler_, params_, query_params_), 0);
    EXPECT_EQ(found_handler_, handler2);

    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/test/nested/path", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_, handler3);

    // Test finding non-existent route
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/nonexistent", found_handler_, params_,
                                  query_params_),
              -1);
}

TEST_F(HttpRouterTest, ParameterExtraction)
{
    auto handler = std::make_shared<DummyHandler>(1);

    // Add a route with a parameter
    router_->add_route(HttpMethod::GET, "/users/:id", handler);

    // Test parameter extraction
    EXPECT_EQ(
        router_->find_route(HttpMethod::GET, "/users/123", found_handler_, params_, query_params_),
        0);
    EXPECT_EQ(found_handler_, handler);
    EXPECT_EQ(params_["id"], "123");
}

TEST_F(HttpRouterTest, MultipleParameters)
{
    auto handler = std::make_shared<DummyHandler>(1);

    // Add a route with multiple parameters
    router_->add_route(HttpMethod::GET, "/users/:userId/posts/:postId", handler);

    // Test multiple parameter extraction
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/users/123/posts/456", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_, handler);
    EXPECT_EQ(params_["userId"], "123");
    EXPECT_EQ(params_["postId"], "456");
}

TEST_F(HttpRouterTest, WildcardWithParameters)
{
    auto handler = std::make_shared<DummyHandler>(1);

    // Add a route with wildcard and parameters
    router_->add_route(HttpMethod::GET, "/files/:path/*", handler);

    // Test wildcard with parameter extraction
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/files/documents/report.pdf", found_handler_,
                                  params_, query_params_),
              0);
    EXPECT_EQ(found_handler_, handler);
    EXPECT_EQ(params_["path"], "documents");
    EXPECT_EQ(params_["*"], "report.pdf");
}

TEST_F(HttpRouterTest, QueryParameters)
{
    auto handler = std::make_shared<DummyHandler>(1);

    // Add a static route
    router_->add_route(HttpMethod::GET, "/search", handler);

    // Test basic query parameter extraction
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/search?q=test&page=2", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_, handler);
    EXPECT_EQ(query_params_["q"], "test");
    EXPECT_EQ(query_params_["page"], "2");

    // Clear params for next test
    params_.clear();
    query_params_.clear();

    // Test URL-encoded query parameters
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/search?q=hello+world&filter=category%3Dbooks",
                                  found_handler_, params_, query_params_),
              0);
    EXPECT_EQ(found_handler_, handler);
    EXPECT_EQ(query_params_["q"], "hello world");
    EXPECT_EQ(query_params_["filter"], "category=books");
}

TEST_F(HttpRouterTest, PathAndQueryParameters)
{
    auto handler = std::make_shared<DummyHandler>(1);

    // Add a route with path parameters
    router_->add_route(HttpMethod::GET, "/users/:userId/posts/:postId", handler);

    // Test combination of path and query parameters
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/users/123/posts/456?sort=date&order=desc",
                                  found_handler_, params_, query_params_),
              0);
    EXPECT_EQ(found_handler_, handler);

    // Check path parameters
    EXPECT_EQ(params_["userId"], "123");
    EXPECT_EQ(params_["postId"], "456");

    // Check query parameters
    EXPECT_EQ(query_params_["sort"], "date");
    EXPECT_EQ(query_params_["order"], "desc");
}

TEST_F(HttpRouterTest, QueryParametersWithoutValue)
{
    auto handler = std::make_shared<DummyHandler>(1);

    // Add a static route
    router_->add_route(HttpMethod::GET, "/options", handler);

    // Test query parameters without values
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/options?debug&verbose", found_handler_,
                                  params_, query_params_),
              0);
    EXPECT_EQ(found_handler_, handler);
    EXPECT_EQ(query_params_["debug"], "");
    EXPECT_EQ(query_params_["verbose"], "");

    // Test mixed query parameters with and without values
    params_.clear();
    query_params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/options?debug&level=info", found_handler_,
                                  params_, query_params_),
              0);
    EXPECT_EQ(found_handler_, handler);
    EXPECT_EQ(query_params_["debug"], "");
    EXPECT_EQ(query_params_["level"], "info");
}

// Test hybrid routing strategy - hash map for short paths, trie for long paths
TEST_F(HttpRouterTest, HybridRoutingStrategy)
{
    // Create handlers with IDs for differentiation
    auto short_handler = std::make_shared<DummyHandler>(1); // Hash table storage
    auto long_handler = std::make_shared<DummyHandler>(2);  // Trie storage
    auto param_handler = std::make_shared<DummyHandler>(3); // Parameterized routes

    // Add various types of routes
    router_->add_route(HttpMethod::GET, "/api", short_handler); // Short -> hash
    router_->add_route(HttpMethod::GET, "/api/users/profiles/settings/notifications",
                       long_handler); // Long -> trie
    router_->add_route(HttpMethod::GET, "/products/:category/:id",
                       param_handler); // Parameterized

    // Test short path lookup (hash table)
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/api", found_handler_, params_, query_params_),
              0);
    ASSERT_NE(found_handler_, nullptr);
    EXPECT_EQ(found_handler_->id(), 1);

    // Test long path lookup (trie)
    params_.clear();
    query_params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/api/users/profiles/settings/notifications",
                                  found_handler_, params_, query_params_),
              0);
    ASSERT_NE(found_handler_, nullptr);
    EXPECT_EQ(found_handler_->id(), 2);

    // Test parameterized route lookup
    params_.clear();
    query_params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/products/electronics/12345", found_handler_,
                                  params_, query_params_),
              0);
    ASSERT_NE(found_handler_, nullptr);
    EXPECT_EQ(found_handler_->id(), 3);
    EXPECT_EQ(params_["category"], "electronics");
    EXPECT_EQ(params_["id"], "12345");
}

TEST_F(HttpRouterTest, ComplexRoutingScenario)
{
    // Add multiple routes with various characteristics
    std::vector<std::shared_ptr<DummyHandler>> handlers;
    for (int i = 0; i < 50; ++i) {
        handlers.push_back(std::make_shared<DummyHandler>(i));
    }

    // Short path routes
    router_->add_route(HttpMethod::GET, "/", handlers[0]);
    router_->add_route(HttpMethod::GET, "/home", handlers[1]);
    router_->add_route(HttpMethod::GET, "/about", handlers[2]);
    router_->add_route(HttpMethod::GET, "/contact", handlers[3]);
    router_->add_route(HttpMethod::POST, "/login", handlers[4]);
    router_->add_route(HttpMethod::POST, "/signup", handlers[5]);

    // Long path routes (trie)
    router_->add_route(HttpMethod::GET, "/api/v1/users/profiles/settings", handlers[10]);
    router_->add_route(HttpMethod::GET, "/api/v1/users/profiles/photos", handlers[11]);
    router_->add_route(HttpMethod::GET, "/api/v1/users/profiles/friends", handlers[12]);
    router_->add_route(HttpMethod::GET, "/api/v1/posts/recent/comments", handlers[13]);
    router_->add_route(HttpMethod::GET, "/api/v1/posts/trending/today", handlers[14]);

    // Parameterized routes
    router_->add_route(HttpMethod::GET, "/users/:userId", handlers[20]);
    router_->add_route(HttpMethod::PUT, "/users/:userId/profile", handlers[21]);
    router_->add_route(HttpMethod::DELETE, "/users/:userId/posts/:postId", handlers[22]);
    router_->add_route(HttpMethod::GET, "/products/:category/:productId/reviews", handlers[23]);

    // Wildcard routes
    router_->add_route(HttpMethod::GET, "/static/*", handlers[30]);
    router_->add_route(HttpMethod::GET, "/files/:type/*", handlers[31]);

    // Test wildcard
    EXPECT_EQ(
        router_->find_route(HttpMethod::GET, "/static/123", found_handler_, params_, query_params_),
        0);
    EXPECT_EQ(found_handler_->id(), 30);
    EXPECT_EQ(params_["*"], "123");

    // Test short path
    EXPECT_EQ(
        router_->find_route(HttpMethod::GET, "/about", found_handler_, params_, query_params_), 0);
    EXPECT_EQ(found_handler_->id(), 2);

    // Test long path
    params_.clear();
    query_params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/api/v1/posts/trending/today", found_handler_,
                                  params_, query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 14);

    // Test parameterized route
    params_.clear();
    query_params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::DELETE, "/users/42/posts/123", found_handler_,
                                  params_, query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 22);
    EXPECT_EQ(params_["userId"], "42");
    EXPECT_EQ(params_["postId"], "123");

    // Test wildcard route with parameters
    params_.clear();
    query_params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/files/documents/reports/annual/2023.pdf",
                                  found_handler_, params_, query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 31);
    EXPECT_EQ(params_["type"], "documents");
    EXPECT_EQ(params_["*"], "reports/annual/2023.pdf");

    // Test mixed path and query parameters
    params_.clear();
    query_params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::GET,
                                  "/products/electronics/12345/reviews?sort=newest&page=2",
                                  found_handler_, params_, query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 23);
    EXPECT_EQ(params_["category"], "electronics");
    EXPECT_EQ(params_["productId"], "12345");
    EXPECT_EQ(query_params_["sort"], "newest");
    EXPECT_EQ(query_params_["page"], "2");
}

TEST_F(HttpRouterTest, RouteConflictResolution)
{
    // Create handlers
    auto handler1 = std::make_shared<DummyHandler>(1);
    auto handler2 = std::make_shared<DummyHandler>(2);
    auto handler3 = std::make_shared<DummyHandler>(3);

    // Add potentially conflicting routes
    router_->add_route(HttpMethod::GET, "/api/users", handler1);
    router_->add_route(HttpMethod::GET, "/api/:resource", handler2);
    router_->add_route(HttpMethod::GET, "/api/users/:id", handler3);

    // Test static route takes priority over parameterized route
    EXPECT_EQ(
        router_->find_route(HttpMethod::GET, "/api/users", found_handler_, params_, query_params_),
        0);
    EXPECT_EQ(found_handler_->id(), 1);

    // Test parameterized route matches other resources
    params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/api/products", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 2);
    EXPECT_EQ(params_["resource"], "products");

    // Test more specific parameterized route
    params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/api/users/42", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 3);
    EXPECT_EQ(params_["id"], "42");
}

TEST_F(HttpRouterTest, PerformanceBenchmark)
{
    // Add many routes for performance testing
    constexpr int kNumRoutes = 1000;
    std::vector<std::shared_ptr<DummyHandler>> handlers;

    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.push_back(std::make_shared<DummyHandler>(i));
        HttpMethod method = static_cast<HttpMethod>(i % 5);
        if (method == HttpMethod::UNKNOWN) {
            method = HttpMethod::GET;
        }

        // Add various types of routes
        if (i % 5 == 0) {
            // Short paths
            router_->add_route(method, "/short" + std::to_string(i), handlers[i]);
        } else if (i % 5 == 1) {
            // Long paths
            router_->add_route(method, "/api/v1/users/profiles/settings/" + std::to_string(i),
                               handlers[i]);
        } else if (i % 5 == 2) {
            // Parameterized routes
            router_->add_route(method, "/users/" + std::to_string(i) + "/:param", handlers[i]);
        } else if (i % 5 == 3) {
            // Multiple parameter routes
            router_->add_route(method, "/products/:category/" + std::to_string(i) + "/:id",
                               handlers[i]);
        } else {
            // Wildcard routes
            router_->add_route(method, "/files/" + std::to_string(i) + "/*", handlers[i]);
        }
    }

    // Execute some route matching to verify performance
    EXPECT_EQ(
        router_->find_route(HttpMethod::GET, "/short100", found_handler_, params_, query_params_),
        0);
    EXPECT_EQ(found_handler_->id(), 100);

    params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::POST, "/api/v1/users/profiles/settings/101",
                                  found_handler_, params_, query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 101);
}

TEST_F(HttpRouterTest, RouteCaching)
{
    // Create route handlers
    auto handler1 = std::make_shared<DummyHandler>(1);
    auto handler2 = std::make_shared<DummyHandler>(2);
    auto handler3 = std::make_shared<DummyHandler>(3);

    // Add various types of routes
    router_->add_route(HttpMethod::GET, "/api/users", handler1);
    router_->add_route(HttpMethod::POST, "/api/posts/:postId", handler2);
    router_->add_route(HttpMethod::GET, "/files/documents/*", handler3);

    // First lookup - should require regular route matching
    auto start_time = std::chrono::high_resolution_clock::now();
    EXPECT_EQ(router_->find_route(HttpMethod::POST, "/api/posts/123", found_handler_, params_,
                                  query_params_),
              0);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto first_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    EXPECT_EQ(found_handler_->id(), 2);
    EXPECT_EQ(params_["postId"], "123");

    // Clear parameters
    params_.clear();

    // Second lookup of same path - should hit cache
    start_time = std::chrono::high_resolution_clock::now();
    EXPECT_EQ(router_->find_route(HttpMethod::POST, "/api/posts/123", found_handler_, params_,
                                  query_params_),
              0);
    end_time = std::chrono::high_resolution_clock::now();
    auto second_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    EXPECT_EQ(found_handler_->id(), 2);
    EXPECT_EQ(params_["postId"], "123");

    std::cout << "First lookup: " << first_duration << " ns" << std::endl;
    std::cout << "Second lookup (cached): " << second_duration << " ns" << std::endl;

    // Test wildcard route caching
    params_.clear();
    query_params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/files/documents/report.pdf", found_handler_,
                                  params_, query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 3);
    EXPECT_EQ(params_["*"], "report.pdf");
}

TEST_F(HttpRouterTest, AddAndFindWithDifferentMethods)
{
    auto get_handler = std::make_shared<DummyHandler>(1);
    auto post_handler = std::make_shared<DummyHandler>(2);
    auto put_handler = std::make_shared<DummyHandler>(3);
    auto delete_handler = std::make_shared<DummyHandler>(4);

    // Add same path with different methods
    router_->add_route(HttpMethod::GET, "/api/resource", get_handler);
    router_->add_route(HttpMethod::POST, "/api/resource", post_handler);
    router_->add_route(HttpMethod::PUT, "/api/resource", put_handler);
    router_->add_route(HttpMethod::DELETE, "/api/resource", delete_handler);

    // Test each method finds correct handler
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/api/resource", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 1);

    EXPECT_EQ(router_->find_route(HttpMethod::POST, "/api/resource", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 2);

    EXPECT_EQ(router_->find_route(HttpMethod::PUT, "/api/resource", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 3);

    EXPECT_EQ(router_->find_route(HttpMethod::DELETE, "/api/resource", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 4);

    // Test non-registered method
    EXPECT_EQ(router_->find_route(HttpMethod::PATCH, "/api/resource", found_handler_, params_,
                                  query_params_),
              -1);
}

TEST_F(HttpRouterTest, MethodSpecificCache)
{
    auto get_handler = std::make_shared<DummyHandler>(1);
    auto post_handler = std::make_shared<DummyHandler>(2);

    router_->add_route(HttpMethod::GET, "/api/:id", get_handler);
    router_->add_route(HttpMethod::POST, "/api/:id", post_handler);

    // First access to warm up cache
    EXPECT_EQ(
        router_->find_route(HttpMethod::GET, "/api/123", found_handler_, params_, query_params_),
        0);
    EXPECT_EQ(found_handler_->id(), 1);
    EXPECT_EQ(params_["id"], "123");

    params_.clear();
    EXPECT_EQ(
        router_->find_route(HttpMethod::POST, "/api/123", found_handler_, params_, query_params_),
        0);
    EXPECT_EQ(found_handler_->id(), 2);
    EXPECT_EQ(params_["id"], "123");

    // Test cached access for each method
    params_.clear();
    EXPECT_EQ(
        router_->find_route(HttpMethod::GET, "/api/123", found_handler_, params_, query_params_),
        0);
    EXPECT_EQ(found_handler_->id(), 1);
    EXPECT_EQ(params_["id"], "123");

    params_.clear();
    EXPECT_EQ(
        router_->find_route(HttpMethod::POST, "/api/123", found_handler_, params_, query_params_),
        0);
    EXPECT_EQ(found_handler_->id(), 2);
    EXPECT_EQ(params_["id"], "123");
}

TEST_F(HttpRouterTest, AddRouteWithUnknownMethod)
{
    auto handler = std::make_shared<DummyHandler>(1);

    // Adding route with UNKNOWN method should still work
    router_->add_route(HttpMethod::UNKNOWN, "/unknown", handler);

    // Should be findable
    EXPECT_EQ(router_->find_route(HttpMethod::UNKNOWN, "/unknown", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 1);
}
