/**
 * @file http_router_stress_test.cpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief Stress tests for HTTP router performance under high load
 * HTTP路由器高负载性能压力测试
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
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <string>
#include <thread>
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
    void operator()() const
    {
        // Implementation of operator()
    }

    int id() const { return id_; }

private:
    int id_ = 0;
};

class RouterStressTest : public ::testing::Test
{
protected:
    void SetUp() override { router_ = std::make_unique<router<DummyHandler>>(); }

    std::unique_ptr<router<DummyHandler>> router_;
};

// Test performance with large number of routes
TEST_F(RouterStressTest, LargeNumberOfRoutes)
{
    constexpr int kNumRoutes = 10000;
    std::vector<DummyHandler> handlers;

    // Add many routes
    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.emplace_back(i);

        if (i % 4 == 0) {
            // Static routes
            router_->add_route(HttpMethod::GET, "/static" + std::to_string(i),
                               std::move(handlers[i]));
        } else if (i % 4 == 1) {
            // Parameterized routes
            router_->add_route(HttpMethod::GET, "/users/" + std::to_string(i) + "/:id",
                               std::move(handlers[i]));
        } else if (i % 4 == 2) {
            // Long paths for trie
            router_->add_route(HttpMethod::GET,
                               "/api/v1/users/profiles/settings/advanced/" + std::to_string(i),
                               std::move(handlers[i]));
        } else {
            // Wildcard routes
            router_->add_route(HttpMethod::GET, "/files/" + std::to_string(i) + "/*",
                               std::move(handlers[i]));
        }
    }

    // Test lookup performance
    auto start_time = std::chrono::high_resolution_clock::now();

    // Perform many lookups
    for (int i = 0; i < 1000; ++i) {
        int route_idx = i % kNumRoutes;

        if (route_idx % 4 == 0) {
            std::map<std::string, std::string> params, query_params;
            router_->find_route(HttpMethod::GET, "/static" + std::to_string(route_idx), params,
                                query_params);
        } else if (route_idx % 4 == 1) {
            std::map<std::string, std::string> params, query_params;
            router_->find_route(HttpMethod::GET, "/users/" + std::to_string(route_idx) + "/123",
                                params, query_params);
        } else if (route_idx % 4 == 2) {
            std::map<std::string, std::string> params, query_params;
            router_->find_route(HttpMethod::GET,
                                "/api/v1/users/profiles/settings/advanced/" +
                                    std::to_string(route_idx),
                                params, query_params);
        } else {
            std::map<std::string, std::string> params, query_params;
            router_->find_route(HttpMethod::GET,
                                "/files/" + std::to_string(route_idx) + "/test.txt", params,
                                query_params);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    std::cout << "Large routes stress test: " << total_time / 1000.0 << " μs per lookup"
              << std::endl;

    // Performance should be reasonable even with many routes
    EXPECT_LT(total_time / 1000.0, 1000.0); // Less than 1ms per lookup
}

// Test memory usage under stress
TEST_F(RouterStressTest, MemoryStressTest)
{
    constexpr int kNumRoutes = 5000;
    std::vector<DummyHandler> handlers;

    // Add routes with varying complexity
    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.emplace_back(i);

        // Create paths with different characteristics
        std::string base_path = "/complex/path/with/many/segments/" + std::to_string(i);

        if (i % 3 == 0) {
            // Add parameters
            base_path += "/:param1/:param2/:param3";
        } else if (i % 3 == 1) {
            // Add wildcard
            base_path += "/*";
        }
        // else: keep as static route

        router_->add_route(HttpMethod::GET, base_path, std::move(handlers[i]));
    }

    // Test that we can still perform lookups efficiently
    for (int i = 0; i < 100; ++i) {
        std::string test_path = "/complex/path/with/many/segments/" + std::to_string(i * 50);

        if ((i * 50) % 3 == 0) {
            test_path += "/value1/value2/value3";
        } else if ((i * 50) % 3 == 1) {
            test_path += "/some/file.txt";
        }

        std::map<std::string, std::string> params, query_params;
        auto result = router_->find_route(HttpMethod::GET, test_path, params, query_params);
        EXPECT_TRUE(result.has_value()) << "Route not found in memory stress test";
    }
}

// Test concurrent access (basic thread safety check)
TEST_F(RouterStressTest, ConcurrentAccessTest)
{
    constexpr int kNumRoutes = 1000;
    constexpr int kNumThreads = 4;
    constexpr int kLookupsPerThread = 100;

    // Add routes
    std::vector<DummyHandler> handlers;
    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.emplace_back(i);
        router_->add_route(HttpMethod::GET, "/route" + std::to_string(i), std::move(handlers[i]));
    }

    // Function for each thread to execute
    auto thread_func = [this, kNumRoutes, kLookupsPerThread, &handlers]() {
        for (int i = 0; i < kLookupsPerThread; ++i) {
            int route_idx = std::rand() % kNumRoutes;
            std::map<std::string, std::string> params, query_params;
            router_->find_route(HttpMethod::GET, "/route" + std::to_string(route_idx), params,
                                query_params);
        }
    };

    // Launch threads
    std::vector<std::thread> threads;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back(thread_func);
    }

    // Wait for all threads to complete
    for (auto &thread : threads) {
        thread.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    std::cout << "Concurrent access test completed in " << total_time << " ms" << std::endl;

    // Should complete in reasonable time
    EXPECT_LT(total_time, 5000); // Less than 5 seconds
}

// Test cache performance under stress
TEST_F(RouterStressTest, CacheStressTest)
{
    constexpr int kNumRoutes = 2000;

    // Add routes
    std::vector<DummyHandler> handlers;
    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.emplace_back(i);
        router_->add_route(HttpMethod::GET, "/cached" + std::to_string(i), std::move(handlers[i]));
    }

    // First pass - populate cache
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kNumRoutes; ++i) {
        std::map<std::string, std::string> params, query_params;
        router_->find_route(HttpMethod::GET, "/cached" + std::to_string(i), params, query_params);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto first_pass_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    // Second pass - should benefit from cache
    start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kNumRoutes; ++i) {
        std::map<std::string, std::string> params, query_params;
        router_->find_route(HttpMethod::GET, "/cached" + std::to_string(i), params, query_params);
    }
    end_time = std::chrono::high_resolution_clock::now();
    auto second_pass_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    std::cout << "Cache stress test - First pass: " << first_pass_time / 1000.0 << " ms"
              << std::endl;
    std::cout << "Cache stress test - Second pass: " << second_pass_time / 1000.0 << " ms"
              << std::endl;

    if (second_pass_time > 0) {
        double speedup = static_cast<double>(first_pass_time) / second_pass_time;
        std::cout << "Cache speedup: " << speedup << "x" << std::endl;

        // Second pass should not be significantly slower
        // At microsecond level, timing can vary due to measurement noise
        EXPECT_GE(speedup, 0.8); // Allow for some variance in timing
    }
}