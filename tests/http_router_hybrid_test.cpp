/**
 * @file http_router_hybrid_test.cpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief Tests for hybrid routing strategies in HTTP router
 * HTTP路由器混合路由策略测试
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
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

using namespace co;

class DummyHandler
{
public:
    DummyHandler() = default;
    ~DummyHandler() = default;

    // Copy and move operations following Google style
    DummyHandler(const DummyHandler &) = default;
    DummyHandler &operator=(const DummyHandler &) = default;
    DummyHandler(DummyHandler &&) = default;
    DummyHandler &operator=(DummyHandler &&) = default;

    explicit DummyHandler(int id) : id_(id) {}
    // 仿函数调用操作符（满足CallableHandler约束）
    void operator()() const {}

    int id() const { return id_; }

private:
    int id_ = 0;
};

class HybridRoutingTest : public ::testing::Test
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

// Test basic hybrid routing functionality
TEST_F(HybridRoutingTest, BasicHybridRouting)
{
    // Add different types of routes
    auto h1 = DummyHandler(1);
    auto h2 = DummyHandler(2);
    auto h3 = DummyHandler(3);

    // These routes should be stored in hash table
    router_->add_route(HttpMethod::GET, "/api", std::move(h1));   // Short path
    router_->add_route(HttpMethod::GET, "/about", std::move(h1)); // Short path
    router_->add_route(HttpMethod::GET, "/login", std::move(h1)); // Short path

    // These routes should be stored in Trie tree
    router_->add_route(HttpMethod::GET, "/api/users/profiles/settings", std::move(h2)); // Long path
    router_->add_route(HttpMethod::GET, "/api/users/profiles/photos", std::move(h2));   // Long path
    router_->add_route(HttpMethod::GET, "/api/users/profiles/friends/requests",
                       std::move(h2)); // Long path

    // These should be stored as parameterized routes
    router_->add_route(HttpMethod::GET, "/users/:userId", std::move(h3)); // Parameterized route
    router_->add_route(HttpMethod::GET, "/api/posts/:postId/comments",
                       std::move(h3));                              // Parameterized route
    router_->add_route(HttpMethod::GET, "/files/*", std::move(h3)); // Wildcard route

    // Test hash table routes
    auto result = router_->find_route(HttpMethod::GET, "/api", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);

    // Test Trie tree routes
    params_.clear();
    result = router_->find_route(HttpMethod::GET, "/api/users/profiles/settings", params_,
                                 query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 2);

    // Test parameterized routes
    params_.clear();
    result = router_->find_route(HttpMethod::GET, "/users/42", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 3);
    EXPECT_EQ(params_["userId"], "42");
}

// Test efficiency of hybrid routing system
TEST_F(HybridRoutingTest, RoutingEfficiency)
{
    // Add many routes to test efficiency
    constexpr int kNumRoutes = 1000;
    std::vector<DummyHandler> handlers;

    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.push_back(DummyHandler(i));
    }

    // Add short paths (hash table)
    for (int i = 0; i < 200; ++i) {
        std::string path = "/short" + std::to_string(i);
        router_->add_route(HttpMethod::GET, path, std::move(handlers[i]));
    }

    // Add long paths (Trie tree)
    for (int i = 200; i < 500; ++i) {
        std::string path = "/api/users/profiles/settings/" + std::to_string(i);
        router_->add_route(HttpMethod::GET, path, std::move(handlers[i]));
    }

    // Add parameterized routes
    for (int i = 500; i < 800; ++i) {
        std::string path = "/users/" + std::to_string(i) + "/:id";
        router_->add_route(HttpMethod::GET, path, std::move(handlers[i]));
    }

    // Add wildcard routes
    for (int i = 800; i < kNumRoutes; ++i) {
        std::string path = "/files/" + std::to_string(i) + "/*";
        router_->add_route(HttpMethod::GET, path, std::move(handlers[i]));
    }

    // Test hash table route lookup performance
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 200; ++i) {
        std::string path = "/short" + std::to_string(i);
        auto result = router_->find_route(HttpMethod::GET, path, params_, query_params_);
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value().get().id(), i);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto hash_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    // Test Trie tree route lookup performance
    start_time = std::chrono::high_resolution_clock::now();
    for (int i = 200; i < 500; ++i) {
        std::string path = "/api/users/profiles/settings/" + std::to_string(i);
        auto result = router_->find_route(HttpMethod::GET, path, params_, query_params_);
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value().get().id(), i);
    }
    end_time = std::chrono::high_resolution_clock::now();
    auto trie_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    // Test parameterized route lookup performance
    start_time = std::chrono::high_resolution_clock::now();
    for (int i = 500; i < 800; ++i) {
        std::string path = "/users/" + std::to_string(i) + "/123";
        auto result = router_->find_route(HttpMethod::GET, path, params_, query_params_);
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value().get().id(), i);
    }
    end_time = std::chrono::high_resolution_clock::now();
    auto param_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    std::cout << "Hash table lookup: " << hash_time / 200.0 << " μs per lookup" << std::endl;
    std::cout << "Trie tree lookup: " << trie_time / 300.0 << " μs per lookup" << std::endl;
    std::cout << "Parameterized lookup: " << param_time / 300.0 << " μs per lookup" << std::endl;

    // Hash table routes should be fastest, then Trie tree, then parameterized routes
    EXPECT_LE(hash_time / 200.0, trie_time / 300.0);
    EXPECT_LE(trie_time / 300.0, param_time / 300.0);
}

// Test route lookup priority
TEST_F(HybridRoutingTest, RoutingPriority)
{
    // Create route handlers
    auto static_handler = DummyHandler(1);
    auto param_handler = DummyHandler(2);

    // Add static and parameterized routes to same path pattern
    router_->add_route(HttpMethod::GET, "/api/users", std::move(static_handler)); // Static route
    router_->add_route(HttpMethod::GET, "/api/:resource",
                       std::move(param_handler)); // Parameterized route

    // Test route matching priority - static routes should take priority over parameterized routes
    auto result = router_->find_route(HttpMethod::GET, "/api/users", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1); // Should match static route

    // Test parameterized route
    params_.clear();
    result = router_->find_route(HttpMethod::GET, "/api/products", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 2); // Should match parameterized route
    EXPECT_EQ(params_["resource"], "products");
}

// Test segment count matching optimization effectiveness
TEST_F(HybridRoutingTest, SegmentIndexingOptimization)
{
    // Create many parameterized routes with different segment counts
    auto handler1 = DummyHandler(1);
    auto handler2 = DummyHandler(2);
    auto handler3 = DummyHandler(3);

    // 1-segment routes
    router_->add_route(HttpMethod::GET, "/:param", std::move(handler1));

    // 2-segment routes
    for (int i = 0; i < 100; ++i) {
        router_->add_route(HttpMethod::GET, "/test" + std::to_string(i) + "/:param",
                           std::move(handler1));
    }

    // 3-segment routes
    for (int i = 0; i < 100; ++i) {
        router_->add_route(HttpMethod::GET, "/api/:resource/:id" + std::to_string(i),
                           std::move(handler2));
    }

    // 4-segment routes
    for (int i = 0; i < 100; ++i) {
        router_->add_route(HttpMethod::GET, "/api/v1/:resource/:id" + std::to_string(i),
                           std::move(handler3));
    }

    // Test 3-segment route matching efficiency - should directly lookup 3-segment routes
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 20; ++i) {
        params_.clear();
        auto result = router_->find_route(HttpMethod::GET, "/api/users/id" + std::to_string(i),
                                          params_, query_params_);
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value().get().id(), 2);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto segment_optimized_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    std::cout << "Segment index optimization: " << segment_optimized_time / 20.0 << " μs per lookup"
              << std::endl;

    // Performance test, no hard assertions
}

// Test long path prefix sharing efficiency
TEST_F(HybridRoutingTest, TriePrefixSharing)
{
    // Create many routes with common prefixes
    constexpr int kNumRoutes = 1000;
    std::vector<DummyHandler> handlers;

    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.push_back(DummyHandler(i));

        // All routes share "/api/v1/users/profiles" prefix
        std::string path = "/api/v1/users/profiles/setting" + std::to_string(i);
        router_->add_route(HttpMethod::GET, path, std::move(handlers[i]));
    }

    // Randomly access some routes, measure performance
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, kNumRoutes - 1);

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; ++i) {
        int idx = distrib(gen);
        std::string path = "/api/v1/users/profiles/setting" + std::to_string(idx);
        auto result = router_->find_route(HttpMethod::GET, path, params_, query_params_);
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value().get().id(), idx);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto trie_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    std::cout << "Trie tree with shared prefixes: " << trie_time / 100.0 << " μs per lookup"
              << std::endl;

    // Performance test only, no hard assertions
}
