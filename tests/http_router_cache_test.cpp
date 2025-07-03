/**
 * @file http_router_cache_test.cpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief Unit tests for HTTP router caching functionality
 * 路由器缓存功能的单元测试
 * @version 0.1
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 */

#include "../include/router/router.hpp"
#include "../include/router/router_group.hpp"
#include "../include/router/router_impl.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace flc;

class DummyHandler
{
public:
    DummyHandler() = default;
    ~DummyHandler() = default;

    // Copy and move constructors following Google style
    DummyHandler(const DummyHandler &) = default;
    DummyHandler &operator=(const DummyHandler &) = default;
    DummyHandler(DummyHandler &&) = default;
    DummyHandler &operator=(DummyHandler &&) = default;

    explicit DummyHandler(int id) : id_(id) {}
    // 仿函数调用操作符（满足CallableHandler约束）
    void operator()() const tests/http_router_cache_test.cpp

    int id() const { return id_; }

private:
    int id_ = 0;
};

class RouterCacheTest : public ::testing::Test
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

// Basic cache functionality test
TEST_F(RouterCacheTest, BasicCacheFunctionality)
{
    // Add different types of routes
    auto handler1 = std::make_shared<DummyHandler>(1);
    auto handler2 = std::make_shared<DummyHandler>(2);
    auto handler3 = std::make_shared<DummyHandler>(3);

    router_->add_route(HttpMethod::GET, "/static/path", handler1);   // Static route (hash table)
    router_->add_route(HttpMethod::GET, "/api/users/:id", handler2); // Parameterized route
    router_->add_route(HttpMethod::GET, "/files/documents/*", handler3); // Wildcard route

    // First access - requires route lookup
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/api/users/123", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 2);
    EXPECT_EQ(params_["id"], "123");

    // Clear parameters, access same path again should hit cache and populate parameters
    params_.clear();
    EXPECT_EQ(router_->find_route(HttpMethod::GET, "/api/users/123", found_handler_, params_,
                                  query_params_),
              0);
    EXPECT_EQ(found_handler_->id(), 2);
    EXPECT_EQ(params_["id"], "123"); // Verify cache restored parameters
}

// Cache clearing test
TEST_F(RouterCacheTest, CacheClearTest)
{
    auto handler = std::make_shared<DummyHandler>(1);
    router_->add_route(HttpMethod::GET, "/test/:param", handler);

    // First access
    EXPECT_EQ(
        router_->find_route(HttpMethod::GET, "/test/value", found_handler_, params_, query_params_),
        0);
    EXPECT_EQ(params_["param"], "value");

    // Clear cache
    router_->clear_cache();

    // Clear parameters
    params_.clear();

    // Access again, should re-match route since cache is cleared
    EXPECT_EQ(
        router_->find_route(HttpMethod::GET, "/test/value", found_handler_, params_, query_params_),
        0);
    EXPECT_EQ(params_["param"], "value");
}

// Route cache performance test
TEST_F(RouterCacheTest, CachePerformanceTest)
{
    // Add 1000 different routes
    constexpr int kNumRoutes = 1000;
    for (int i = 0; i < kNumRoutes; ++i) {
        auto handler = std::make_shared<DummyHandler>(i);

        if (i % 3 == 0) {
            // Static routes
            router_->add_route(HttpMethod::GET, "/path" + std::to_string(i), handler);
        } else if (i % 3 == 1) {
            // Parameterized routes
            router_->add_route(HttpMethod::GET, "/users/" + std::to_string(i) + "/:id", handler);
        } else {
            // Wildcard routes
            router_->add_route(HttpMethod::GET, "/files/" + std::to_string(i) + "/*", handler);
        }
    }

    // Prepare 100 test paths
    std::vector<std::string> test_paths;
    for (int i = 0; i < 100; ++i) {
        if (i % 3 == 0) {
            test_paths.push_back("/path" + std::to_string(i * 3));
        } else if (i % 3 == 1) {
            test_paths.push_back("/users/" + std::to_string(i * 3 + 1) + "/test_id");
        } else {
            test_paths.push_back("/files/" + std::to_string(i * 3 + 2) + "/some/deep/path");
        }
    }

    // First round - no cache matching
    auto start = std::chrono::high_resolution_clock::now();

    for (const auto &path : test_paths) {
        params_.clear();
        router_->find_route(HttpMethod::GET, path, found_handler_, params_, query_params_);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto no_cache_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Second round - should hit cache
    start = std::chrono::high_resolution_clock::now();

    for (const auto &path : test_paths) {
        params_.clear();
        router_->find_route(HttpMethod::GET, path, found_handler_, params_, query_params_);
    }

    end = std::chrono::high_resolution_clock::now();
    auto with_cache_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "No cache: " << no_cache_time << " μs" << std::endl;
    std::cout << "With cache: " << with_cache_time << " μs" << std::endl;
    if (with_cache_time > 0) {
        std::cout << "Speedup: " << static_cast<double>(no_cache_time) / with_cache_time << "x"
                  << std::endl;
    }

    // Expect cached version to be at least 2x faster
    EXPECT_GT(static_cast<double>(no_cache_time) / with_cache_time, 2.0);
}

// Random access pattern test
TEST_F(RouterCacheTest, RandomAccessPattern)
{
    // Add many routes
    constexpr int kNumRoutes = 2000;
    std::vector<std::shared_ptr<DummyHandler>> handlers;
    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.push_back(std::make_shared<DummyHandler>(i));
        router_->add_route(HttpMethod::GET, "/route/" + std::to_string(i), handlers.back());
    }

    // Random access to routes
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<> distrib(0, kNumRoutes - 1);

    // Initial access, populate cache
    for (int i = 0; i < kNumRoutes; ++i) {
        router_->find_route(HttpMethod::GET, "/route/" + std::to_string(i), found_handler_, params_,
                            query_params_);
    }

    // Random access 1000 times
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        int idx = distrib(g);
        std::string path = "/route/" + std::to_string(idx);
        router_->find_route(HttpMethod::GET, path, found_handler_, params_, query_params_);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto random_access_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // Sequential access 1000 times
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        std::string path = "/route/" + std::to_string(i);
        router_->find_route(HttpMethod::GET, path, found_handler_, params_, query_params_);
    }
    end = std::chrono::high_resolution_clock::now();
    auto sequential_access_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "Random access time: " << random_access_time / 1000.0 << " ns/op" << std::endl;
    std::cout << "Sequential access time: " << sequential_access_time / 1000.0 << " ns/op"
              << std::endl;
}
