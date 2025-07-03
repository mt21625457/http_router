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

using namespace flc;

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
    void operator()() const tests/http_router_stress_test.cpp

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
    std::vector<std::shared_ptr<DummyHandler>> handlers;

    // Add many routes
    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.push_back(std::make_shared<DummyHandler>(i));

        if (i % 4 == 0) {
            // Static routes
            router_->add_route(HttpMethod::GET, "/static" + std::to_string(i), handlers[i]);
        } else if (i % 4 == 1) {
            // Parameterized routes
            router_->add_route(HttpMethod::GET, "/users/" + std::to_string(i) + "/:id",
                               handlers[i]);
        } else if (i % 4 == 2) {
            // Long paths for trie
            router_->add_route(HttpMethod::GET,
                               "/api/v1/users/profiles/settings/advanced/" + std::to_string(i),
                               handlers[i]);
        } else {
            // Wildcard routes
            router_->add_route(HttpMethod::GET, "/files/" + std::to_string(i) + "/*", handlers[i]);
        }
    }

    // Test lookup performance
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Perform many lookups
    for (int i = 0; i < 1000; ++i) {
        int route_idx = i % kNumRoutes;
        params.clear();

        if (route_idx % 4 == 0) {
            router_->find_route(HttpMethod::GET, "/static" + std::to_string(route_idx),
                                found_handler, params, query_params);
        } else if (route_idx % 4 == 1) {
            router_->find_route(HttpMethod::GET, "/users/" + std::to_string(route_idx) + "/123",
                                found_handler, params, query_params);
        } else if (route_idx % 4 == 2) {
            router_->find_route(HttpMethod::GET,
                                "/api/v1/users/profiles/settings/advanced/" +
                                    std::to_string(route_idx),
                                found_handler, params, query_params);
        } else {
            router_->find_route(HttpMethod::GET,
                                "/files/" + std::to_string(route_idx) + "/test.txt", found_handler,
                                params, query_params);
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
    std::vector<std::shared_ptr<DummyHandler>> handlers;

    // Add routes with varying complexity
    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.push_back(std::make_shared<DummyHandler>(i));

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

        router_->add_route(HttpMethod::GET, base_path, handlers[i]);
    }

    // Test that we can still perform lookups efficiently
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    bool all_found = true;
    for (int i = 0; i < 100; ++i) {
        params.clear();
        std::string test_path = "/complex/path/with/many/segments/" + std::to_string(i * 50);

        if ((i * 50) % 3 == 0) {
            test_path += "/value1/value2/value3";
        } else if ((i * 50) % 3 == 1) {
            test_path += "/some/file.txt";
        }

        int result =
            router_->find_route(HttpMethod::GET, test_path, found_handler, params, query_params);
        if (result != 0) {
            all_found = false;
        }
    }

    EXPECT_TRUE(all_found) << "Some routes were not found in memory stress test";
}

// Test concurrent access (basic thread safety check)
TEST_F(RouterStressTest, ConcurrentAccessTest)
{
    constexpr int kNumRoutes = 1000;
    constexpr int kNumThreads = 4;
    constexpr int kLookupsPerThread = 100;

    // Add routes
    std::vector<std::shared_ptr<DummyHandler>> handlers;
    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.push_back(std::make_shared<DummyHandler>(i));
        router_->add_route(HttpMethod::GET, "/route" + std::to_string(i), handlers[i]);
    }

    // Function for each thread to execute
    auto thread_func = [this, kNumRoutes, kLookupsPerThread]() {
        std::shared_ptr<DummyHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, kNumRoutes - 1);

        for (int i = 0; i < kLookupsPerThread; ++i) {
            int route_idx = distrib(gen);
            router_->find_route(HttpMethod::GET, "/route" + std::to_string(route_idx),
                                found_handler, params, query_params);
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
    std::vector<std::shared_ptr<DummyHandler>> handlers;
    for (int i = 0; i < kNumRoutes; ++i) {
        handlers.push_back(std::make_shared<DummyHandler>(i));
        router_->add_route(HttpMethod::GET, "/cached" + std::to_string(i), handlers[i]);
    }

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // First pass - populate cache
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kNumRoutes; ++i) {
        router_->find_route(HttpMethod::GET, "/cached" + std::to_string(i), found_handler, params,
                            query_params);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto first_pass_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    // Second pass - should benefit from cache
    start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kNumRoutes; ++i) {
        router_->find_route(HttpMethod::GET, "/cached" + std::to_string(i), found_handler, params,
                            query_params);
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

        // Second pass should be faster due to caching
        EXPECT_GT(speedup, 1.5); // At least 1.5x faster
    }
}