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
    void operator()() const
    {
        // Implementation of operator()
    }

    int id() const { return id_; }

private:
    int id_ = 0;
};

class HttpRouterCacheTest : public ::testing::Test
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

// Basic cache functionality test
TEST_F(HttpRouterCacheTest, BasicRouteLookup)
{
    router_->add_route(HttpMethod::GET, "/test", DummyHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result = router_->find_route(HttpMethod::GET, "/test", params, query_params);

    EXPECT_TRUE(result.has_value());
}

// Cache clearing test
TEST_F(HttpRouterCacheTest, CacheClearTest)
{
    DummyHandler handler(1);
    router_->add_route(HttpMethod::GET, "/test/:param", std::move(handler));

    // First access
    auto result = router_->find_route(HttpMethod::GET, "/test/value", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params_["param"], "value");

    // Clear cache
    router_->clear_cache();

    // Clear parameters
    params_.clear();

    // Access again, should re-match route since cache is cleared
    result = router_->find_route(HttpMethod::GET, "/test/value", params_, query_params_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params_["param"], "value");
}

// Route cache performance test
TEST_F(HttpRouterCacheTest, CachePerformanceTest)
{
    // Add 1000 different routes
    constexpr int kNumRoutes = 1000;
    std::vector<DummyHandler> handlers;
    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.push_back(DummyHandler(i));

        if (i % 3 == 0) {
            // Static routes
            router_->add_route(HttpMethod::GET, "/path" + std::to_string(i),
                               std::move(handlers[i]));
        } else if (i % 3 == 1) {
            // Parameterized routes
            router_->add_route(HttpMethod::GET, "/users/" + std::to_string(i) + "/:id",
                               std::move(handlers[i]));
        } else {
            // Wildcard routes
            router_->add_route(HttpMethod::GET, "/files/" + std::to_string(i) + "/*",
                               std::move(handlers[i]));
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
        auto result = router_->find_route(HttpMethod::GET, path, params_, query_params_);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto no_cache_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Second round - should hit cache
    start = std::chrono::high_resolution_clock::now();

    for (const auto &path : test_paths) {
        params_.clear();
        auto result = router_->find_route(HttpMethod::GET, path, params_, query_params_);
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
TEST_F(HttpRouterCacheTest, RandomAccessPattern)
{
    // Add many routes
    constexpr int kNumRoutes = 2000;
    std::vector<DummyHandler> handlers;
    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.push_back(DummyHandler(i));
        router_->add_route(HttpMethod::GET, "/route/" + std::to_string(i), std::move(handlers[i]));
    }

    // Random access to routes
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<> distrib(0, kNumRoutes - 1);

    // Initial access, populate cache
    for (int i = 0; i < kNumRoutes; ++i) {
        auto result = router_->find_route(HttpMethod::GET, "/route/" + std::to_string(i), params_,
                                          query_params_);
    }

    // Random access 1000 times
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        int idx = distrib(g);
        std::string path = "/route/" + std::to_string(idx);
        auto result = router_->find_route(HttpMethod::GET, path, params_, query_params_);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto random_access_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // Sequential access 1000 times
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        std::string path = "/route/" + std::to_string(i);
        auto result = router_->find_route(HttpMethod::GET, path, params_, query_params_);
    }
    end = std::chrono::high_resolution_clock::now();
    auto sequential_access_time =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "Random access time: " << random_access_time / 1000.0 << " ns/op" << std::endl;
    std::cout << "Sequential access time: " << sequential_access_time / 1000.0 << " ns/op"
              << std::endl;
}
