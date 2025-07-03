/**
 * @file router_optimization_integration_test.cpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief 路由器优化功能的集成测试 - Integration tests for router optimization features
 * @version 0.2
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 * 本文件包含对router.hpp中集成优化功能的整体测试
 * This file contains integration tests for optimization features in router.hpp
 *
 * 测试重点 Test Focus:
 * 1. 整体路由性能测试
 * 2. 内存使用测试
 * 3. 线程安全测试
 * 4. 边界条件测试
 * 5. 回归测试（确保优化后功能正确）
 */

#include <algorithm>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include "router/router.hpp"

using namespace flc;

/**
 * @brief 测试用的简单处理器类 - Simple handler class for testing
 */
class TestHandler
{
public:
    void operator()()
    {
        // 空实现
    }
};

/**
 * @class RouterOptimizationIntegrationTest
 * @brief 路由器优化集成测试套件 - Router optimization integration test suite
 */
class RouterOptimizationIntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override { router_.reset(new router<TestHandler>()); }

    void TearDown() override
    {
        // 移除缓存清理
    }

    std::unique_ptr<router<TestHandler>> router_;
};

// ============================================================================
// 基本功能验证测试 - Basic Functionality Verification Tests
// ============================================================================

/**
 * @brief 测试基本路由功能 - Test basic routing functionality
 */
TEST_F(RouterOptimizationIntegrationTest, BasicFunctionality)
{
    router_->add_route(HttpMethod::GET, "/test", TestHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result = router_->find_route(HttpMethod::GET, "/test", params, query_params);

    EXPECT_TRUE(result.has_value());
}

/**
 * @brief 测试参数化路由功能 - Test parameterized routing functionality
 */
TEST_F(RouterOptimizationIntegrationTest, ParameterizedRoutes)
{
    router_->add_route(HttpMethod::GET, "/users/:id", TestHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result = router_->find_route(HttpMethod::GET, "/users/123", params, query_params);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params["id"], "123");
}

/**
 * @brief 测试通配符路由功能 - Test wildcard routing functionality
 */
TEST_F(RouterOptimizationIntegrationTest, WildcardRoutes)
{
    router_->add_route(HttpMethod::GET, "/static/*", TestHandler{});

    std::map<std::string, std::string> params, query_params;
    auto result =
        router_->find_route(HttpMethod::GET, "/static/css/style.css", params, query_params);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params["*"], "css/style.css");
}

// ============================================================================
// 性能基准测试 - Performance Benchmark Tests
// ============================================================================

/**
 * @brief 性能基准测试 - Performance benchmark test
 */
TEST_F(RouterOptimizationIntegrationTest, StaticRoutePerformance)
{
    // 添加静态路由
    router_->add_route(HttpMethod::GET, "/api/health", TestHandler{});
    router_->add_route(HttpMethod::GET, "/api/version", TestHandler{});
    router_->add_route(HttpMethod::POST, "/api/login", TestHandler{});

    // 性能测试
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_->find_route(HttpMethod::GET, "/api/health", params, query_params);
        EXPECT_TRUE(result.has_value());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_LT(duration.count(), 1000000); // 1秒内完成
}

TEST_F(RouterOptimizationIntegrationTest, ParameterizedRoutePerformance)
{
    // 添加参数化路由
    router_->add_route(HttpMethod::GET, "/api/users/:id", TestHandler{});
    router_->add_route(HttpMethod::GET, "/api/users/:id/profile", TestHandler{});
    router_->add_route(HttpMethod::GET, "/api/posts/:category/:id", TestHandler{});

    // 性能测试
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        std::map<std::string, std::string> params, query_params;
        std::string path = "/api/users/" + std::to_string(i % 1000);
        auto result = router_->find_route(HttpMethod::GET, path, params, query_params);
        EXPECT_TRUE(result.has_value());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_LT(duration.count(), 2000000); // 2秒内完成
}

TEST_F(RouterOptimizationIntegrationTest, WildcardRoutePerformance)
{
    // 添加通配符路由
    router_->add_route(HttpMethod::GET, "/static/*", TestHandler{});
    router_->add_route(HttpMethod::GET, "/files/:type/*", TestHandler{});

    // 性能测试
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        std::map<std::string, std::string> params, query_params;
        std::string path = "/static/css/style" + std::to_string(i) + ".css";
        auto result = router_->find_route(HttpMethod::GET, path, params, query_params);
        EXPECT_TRUE(result.has_value());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_LT(duration.count(), 2000000); // 2秒内完成
}

TEST_F(RouterOptimizationIntegrationTest, QueryParameterPerformance)
{
    // 添加路由
    router_->add_route(HttpMethod::GET, "/api/search", TestHandler{});

    // 性能测试
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        std::map<std::string, std::string> params, query_params;
        std::string path = "/api/search?q=test&page=" + std::to_string(i);
        auto result = router_->find_route(HttpMethod::GET, path, params, query_params);
        EXPECT_TRUE(result.has_value());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_LT(duration.count(), 2000000); // 2秒内完成
}

TEST_F(RouterOptimizationIntegrationTest, MixedRoutePerformance)
{
    // 添加混合路由
    router_->add_route(HttpMethod::GET, "/api/test", TestHandler{});

    // 性能测试
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_->find_route(HttpMethod::GET, "/api/test", params, query_params);
        EXPECT_TRUE(result.has_value());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_LT(duration.count(), 1000000); // 1秒内完成
}

TEST_F(RouterOptimizationIntegrationTest, LargeRouteSetPerformance)
{
    // 添加大量路由
    for (int i = 0; i < 1000; ++i) {
        std::string static_path = "/api/static/route" + std::to_string(i);
        router_->add_route(HttpMethod::GET, static_path, TestHandler{});

        std::string param_path = "/api/users/" + std::to_string(i) + "/action" + std::to_string(i);
        router_->add_route(HttpMethod::GET, param_path, TestHandler{});

        std::string wildcard_path = "/api/files/" + std::to_string(i) + "/*";
        router_->add_route(HttpMethod::GET, wildcard_path, TestHandler{});
    }

    // 性能测试
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        std::map<std::string, std::string> params, query_params;

        // 测试静态路由
        std::string static_path = "/api/static/route" + std::to_string(i % 1000);
        auto result1 = router_->find_route(HttpMethod::GET, static_path, params, query_params);
        EXPECT_TRUE(result1.has_value());

        // 测试参数化路由
        std::string param_path =
            "/api/users/" + std::to_string(i % 1000) + "/action" + std::to_string(i % 1000);
        auto result2 = router_->find_route(HttpMethod::GET, param_path, params, query_params);
        EXPECT_TRUE(result2.has_value());

        // 测试通配符路由
        std::string wildcard_path = "/api/files/" + std::to_string(i % 1000) + "/docs/readme.txt";
        auto result3 = router_->find_route(HttpMethod::GET, wildcard_path, params, query_params);
        EXPECT_TRUE(result3.has_value());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_LT(duration.count(), 10000000); // 10秒内完成
}

TEST_F(RouterOptimizationIntegrationTest, PathSplittingPerformance)
{
    std::vector<std::string> segments;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100000; ++i) {
        router_->split_path_optimized("/api/v1/users/123/profile/settings", segments);
        EXPECT_EQ(segments.size(), 6);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_LT(duration.count(), 1000000); // 1秒内完成
}

TEST_F(RouterOptimizationIntegrationTest, UrlDecodingPerformance)
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

    EXPECT_LT(duration.count(), 1000000); // 1秒内完成
}

// ============================================================================
// 线程安全测试 - Thread Safety Tests
// ============================================================================

/**
 * @brief 测试多线程并发访问 - Test multi-threaded concurrent access
 */
TEST_F(RouterOptimizationIntegrationTest, ThreadSafety_ConcurrentLookup)
{
    // 添加测试路由 - Add test routes
    for (int i = 0; i < 50; ++i) {
        router_->add_route(HttpMethod::GET, "/api/thread/test" + std::to_string(i), TestHandler{});
        router_->add_route(HttpMethod::GET, "/api/thread/param/:id/test" + std::to_string(i),
                           TestHandler{});
    }

    const int num_threads = 8;
    const int operations_per_thread = 1000;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};

    // 创建多个线程同时进行路由查找 - Create multiple threads for concurrent route lookup
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                // 测试静态路由 - Test static routes
                std::string static_path = "/api/thread/test" + std::to_string(i % 50);
                std::map<std::string, std::string> params, query_params;
                if (router_->find_route(HttpMethod::GET, static_path, params, query_params)
                        .has_value()) {
                    success_count++;
                } else {
                    error_count++;
                }

                // 测试参数化路由 - Test parameterized routes
                std::string param_path = "/api/thread/param/" + std::to_string(t * 1000 + i) +
                                         "/test" + std::to_string(i % 50);
                params.clear();
                query_params.clear();
                if (router_->find_route(HttpMethod::GET, param_path, params, query_params)
                        .has_value()) {
                    success_count++;
                } else {
                    error_count++;
                }
            }
        });
    }

    // 等待所有线程完成 - Wait for all threads to complete
    for (auto &thread : threads) {
        thread.join();
    }

    std::cout << "\n=== Thread Safety Test ===" << std::endl;
    std::cout << "Successful operations: " << success_count.load() << std::endl;
    std::cout << "Failed operations: " << error_count.load() << std::endl;
    std::cout << "Total operations: " << num_threads * operations_per_thread * 2 << std::endl;

    // 验证所有操作都成功 - Verify all operations succeeded
    EXPECT_EQ(success_count.load(), num_threads * operations_per_thread * 2);
    EXPECT_EQ(error_count.load(), 0);
}

// ============================================================================
// 内存和边界测试 - Memory and Boundary Tests
// ============================================================================

/**
 * @brief 测试大数据量处理 - Test large data processing
 */
TEST_F(RouterOptimizationIntegrationTest, Memory_LargeDataHandling)
{
    // 添加路由处理大路径 - Add routes for large paths
    router_->add_route(HttpMethod::GET, "/api/data/:id", TestHandler{});
    router_->add_route(HttpMethod::GET, "/api/files/*", TestHandler{});

    // 测试长路径参数 - Test long path parameters
    std::string long_id(1000, 'a'); // 1000个'a'字符
    std::string long_path = "/api/data/" + long_id;
    std::map<std::string, std::string> params, query_params;
    auto result = router_->find_route(HttpMethod::GET, long_path, params, query_params);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params["id"], long_id);

    // 测试长通配符路径 - Test long wildcard path
    params.clear();
    std::string long_wildcard_path = "/api/files/";
    for (int i = 0; i < 100; ++i) {
        long_wildcard_path += "very_long_directory_name_" + std::to_string(i) + "/";
    }
    long_wildcard_path += "file.txt";

    result = router_->find_route(HttpMethod::GET, long_wildcard_path, params, query_params);
    EXPECT_TRUE(result.has_value());
    EXPECT_GT(params["*"].length(), 1000);

    // 测试大量查询参数 - Test many query parameters
    query_params.clear();
    std::string query_path = "/api/data/test?";
    for (int i = 0; i < 100; ++i) {
        if (i > 0)
            query_path += "&";
        query_path += "param" + std::to_string(i) + "=value" + std::to_string(i);
    }

    result = router_->find_route(HttpMethod::GET, query_path, params, query_params);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(params.size(), 100);
    EXPECT_EQ(params["param50"], "value50");
}

/**
 * @brief 测试路径边界条件 - Test path boundary conditions
 */
TEST_F(RouterOptimizationIntegrationTest, Boundary_PathEdgeCases)
{
    // 添加各种边界情况的路由 - Add routes for various boundary cases
    router_->add_route(HttpMethod::GET, "/", TestHandler{});
    router_->add_route(HttpMethod::GET, "/api", TestHandler{});
    router_->add_route(HttpMethod::GET, "/api/:param", TestHandler{});
    router_->add_route(HttpMethod::GET, "/static/*", TestHandler{});

    // 测试根路径 - Test root path
    std::map<std::string, std::string> params, query_params;
    auto result = router_->find_route(HttpMethod::GET, "/", params, query_params);
    EXPECT_TRUE(result.has_value());

    // 测试连续斜杠 - Test consecutive slashes
    EXPECT_TRUE(router_->find_route(HttpMethod::GET, "//api", params, query_params).has_value());
    EXPECT_TRUE(router_->find_route(HttpMethod::GET, "/api//", params, query_params).has_value());

    // 测试特殊字符参数 - Test special character parameters
    params.clear();
    EXPECT_TRUE(
        router_->find_route(HttpMethod::GET, "/api/user%20name", params, query_params).has_value());
    EXPECT_EQ(params["param"], "user name");

    // 测试空通配符 - Test empty wildcard
    params.clear();
    EXPECT_TRUE(router_->find_route(HttpMethod::GET, "/static/", params, query_params).has_value());
    EXPECT_EQ(params["*"], "");
}

// ============================================================================
// 回归测试 - Regression Tests
// ============================================================================

/**
 * @brief 综合功能回归测试 - Comprehensive functionality regression test
 */
TEST_F(RouterOptimizationIntegrationTest, Regression_ComprehensiveFunctionality)
{
    // 添加各种类型的路由 - Add various types of routes
    // 静态路由 - Static routes
    router_->add_route(HttpMethod::GET, "/api/health", TestHandler{});
    router_->add_route(HttpMethod::POST, "/api/login", TestHandler{});

    // 参数化路由 - Parameterized routes
    router_->add_route(HttpMethod::GET, "/api/users/:id", TestHandler{});
    router_->add_route(HttpMethod::PUT, "/api/users/:id/profile", TestHandler{});

    // 通配符路由 - Wildcard routes
    router_->add_route(HttpMethod::GET, "/static/*", TestHandler{});
    router_->add_route(HttpMethod::GET, "/files/:type/*", TestHandler{});

    // 复杂路由 - Complex routes
    router_->add_route(HttpMethod::GET, "/api/:version/users/:id/posts/:post_id", TestHandler{});

    // 全面测试所有路由类型 - Comprehensive test of all route types
    std::vector<std::tuple<std::string, HttpMethod, int, std::map<std::string, std::string>,
                           std::map<std::string, std::string>>>
        test_cases = {
            // {路径, 方法, 预期处理器ID, 预期参数, 预期查询参数}
            {"/api/health", HttpMethod::GET, 1, {}, {}},
            {"/api/login", HttpMethod::POST, 1, {}, {}},
            {"/api/users/123", HttpMethod::GET, 2, {{"id", "123"}}, {}},
            {"/api/users/456/profile", HttpMethod::PUT, 2, {{"id", "456"}}, {}},
            {"/static/css/main.css", HttpMethod::GET, 3, {{"*", "css/main.css"}}, {}},
            {"/files/images/photo.jpg",
             HttpMethod::GET,
             3,
             {{"type", "images"}, {"*", "photo.jpg"}},
             {}},
            {"/api/v1/users/789/posts/42",
             HttpMethod::GET,
             4,
             {{"version", "v1"}, {"id", "789"}, {"post_id", "42"}},
             {}},
            {"/api/users/123?sort=name&order=asc",
             HttpMethod::GET,
             2,
             {{"id", "123"}},
             {{"sort", "name"}, {"order", "asc"}}},
        };

    std::map<std::string, std::string> params, query_params;
    for (const auto &[path, method, expected_id, expected_params, expected_query] : test_cases) {
        params.clear();
        query_params.clear();

        auto result = router_->find_route(method, path, params, query_params);
        EXPECT_TRUE(result.has_value()) << "Failed to find route for path: " << path;

        EXPECT_EQ(params["id"], std::to_string(expected_id)) << "Wrong handler for path: " << path;

        for (const auto &[key, value] : expected_params) {
            EXPECT_EQ(params[key], value)
                << "Parameter mismatch for path: " << path << ", key: " << key;
        }

        for (const auto &[key, value] : expected_query) {
            EXPECT_EQ(query_params[key], value)
                << "Query parameter mismatch for path: " << path << ", key: " << key;
        }
    }
}

/**
 * @brief 主函数 - Main function
 */
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    std::cout << "\n";
    std::cout << "=========================================\n";
    std::cout << "  路由器优化集成测试套件\n";
    std::cout << "  Router Optimization Integration Test Suite\n";
    std::cout << "=========================================\n";
    std::cout << "\n";

    int result = RUN_ALL_TESTS();

    std::cout << "\n";
    std::cout << "=========================================\n";
    std::cout << "  集成测试完成 - Integration Testing Complete\n";
    std::cout << "=========================================\n";

    return result;
}