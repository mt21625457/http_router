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
private:
    int id_;
    std::string name_;

public:
    explicit TestHandler(int id, std::string name = "") : id_(id), name_(std::move(name)) {}
    // 仿函数调用操作符（满足CallableHandler约束）
    void operator()() const tests/router_optimization_integration_test.cpp

    int id() const { return id_; }
    const std::string &name() const { return name_; }
    void handle() const {}
};

/**
 * @class RouterOptimizationIntegrationTest
 * @brief 路由器优化集成测试套件 - Router optimization integration test suite
 */
class RouterOptimizationIntegrationTest : public ::testing::Test
{
protected:
    router<TestHandler> router_;
    std::shared_ptr<TestHandler> handler_;
    std::map<std::string, std::string> params_;
    std::map<std::string, std::string> query_params_;

    void SetUp() override
    {
        handler_ = std::make_shared<TestHandler>(1, "test_handler");
        params_.clear();
        query_params_.clear();
    }

    void TearDown() override { router_.clear_cache(); }
};

// ============================================================================
// 基本功能验证测试 - Basic Functionality Verification Tests
// ============================================================================

/**
 * @brief 测试基本路由功能 - Test basic routing functionality
 */
TEST_F(RouterOptimizationIntegrationTest, BasicRouting_StaticRoutes)
{
    // 添加静态路由 - Add static routes
    auto static_handler = std::make_shared<TestHandler>(1, "static");
    router_.add_route(HttpMethod::GET, "/api/health", static_handler);
    router_.add_route(HttpMethod::GET, "/api/version", static_handler);
    router_.add_route(HttpMethod::POST, "/api/login", static_handler);

    std::shared_ptr<TestHandler> found_handler;

    // 测试静态路由匹配 - Test static route matching
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/health", found_handler, params_,
                                    query_params_));
    EXPECT_EQ(found_handler->id(), 1);
    EXPECT_EQ(found_handler->name(), "static");

    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/version", found_handler, params_,
                                    query_params_));
    EXPECT_EQ(found_handler->id(), 1);

    EXPECT_EQ(0, router_.find_route(HttpMethod::POST, "/api/login", found_handler, params_,
                                    query_params_));
    EXPECT_EQ(found_handler->id(), 1);

    // 测试不存在的路由 - Test non-existent routes
    EXPECT_EQ(-1, router_.find_route(HttpMethod::GET, "/api/nonexistent", found_handler, params_,
                                     query_params_));
    EXPECT_EQ(-1, router_.find_route(HttpMethod::DELETE, "/api/health", found_handler, params_,
                                     query_params_));
}

/**
 * @brief 测试参数化路由功能 - Test parameterized routing functionality
 */
TEST_F(RouterOptimizationIntegrationTest, BasicRouting_ParameterizedRoutes)
{
    // 添加参数化路由 - Add parameterized routes
    auto param_handler = std::make_shared<TestHandler>(2, "parameterized");
    router_.add_route(HttpMethod::GET, "/api/users/:id", param_handler);
    router_.add_route(HttpMethod::GET, "/api/users/:id/profile", param_handler);
    router_.add_route(HttpMethod::GET, "/api/posts/:category/:id", param_handler);

    std::shared_ptr<TestHandler> found_handler;

    // 测试单参数路由 - Test single parameter route
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/users/123", found_handler, params_,
                                    query_params_));
    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params_["id"], "123");

    // 测试多段参数路由 - Test multi-segment parameter route
    params_.clear();
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/users/456/profile", found_handler,
                                    params_, query_params_));
    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params_["id"], "456");

    // 测试多参数路由 - Test multi-parameter route
    params_.clear();
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/posts/tech/789", found_handler, params_,
                                    query_params_));
    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params_["category"], "tech");
    EXPECT_EQ(params_["id"], "789");
}

/**
 * @brief 测试通配符路由功能 - Test wildcard routing functionality
 */
TEST_F(RouterOptimizationIntegrationTest, BasicRouting_WildcardRoutes)
{
    // 添加通配符路由 - Add wildcard routes
    auto wildcard_handler = std::make_shared<TestHandler>(3, "wildcard");
    router_.add_route(HttpMethod::GET, "/static/*", wildcard_handler);
    router_.add_route(HttpMethod::GET, "/files/:type/*", wildcard_handler);

    std::shared_ptr<TestHandler> found_handler;

    // 测试简单通配符 - Test simple wildcard
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/static/css/main.css", found_handler, params_,
                                    query_params_));
    EXPECT_EQ(found_handler->id(), 3);
    EXPECT_EQ(params_["*"], "css/main.css");

    // 测试混合参数和通配符 - Test mixed parameters and wildcard
    params_.clear();
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/files/images/profile/user123.jpg",
                                    found_handler, params_, query_params_));
    EXPECT_EQ(found_handler->id(), 3);
    EXPECT_EQ(params_["type"], "images");
    EXPECT_EQ(params_["*"], "profile/user123.jpg");
}

// ============================================================================
// URL解码和查询参数测试 - URL Decoding and Query Parameters Tests
// ============================================================================

/**
 * @brief 测试URL解码功能 - Test URL decoding functionality
 */
TEST_F(RouterOptimizationIntegrationTest, UrlDecoding_QueryParameters)
{
    auto handler = std::make_shared<TestHandler>(1, "query_test");
    router_.add_route(HttpMethod::GET, "/api/search", handler);

    std::shared_ptr<TestHandler> found_handler;

    // 测试基本查询参数 - Test basic query parameters
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/search?q=test&limit=10", found_handler,
                                    params_, query_params_));
    EXPECT_EQ(found_handler->id(), 1);
    EXPECT_EQ(query_params_["q"], "test");
    EXPECT_EQ(query_params_["limit"], "10");

    // 测试URL编码的查询参数 - Test URL encoded query parameters
    query_params_.clear();
    EXPECT_EQ(0,
              router_.find_route(HttpMethod::GET, "/api/search?q=hello%20world&message=%21%40%23",
                                 found_handler, params_, query_params_));
    EXPECT_EQ(query_params_["q"], "hello world");
    EXPECT_EQ(query_params_["message"], "!@#");

    // 测试加号编码的空格 - Test plus-encoded spaces
    query_params_.clear();
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/search?q=hello+world&test=a+b+c",
                                    found_handler, params_, query_params_));
    EXPECT_EQ(query_params_["q"], "hello world");
    EXPECT_EQ(query_params_["test"], "a b c");

    // 测试中文字符的URL编码 - Test Chinese characters in URL encoding
    query_params_.clear();
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/search?q=%E4%B8%AD%E6%96%87",
                                    found_handler, params_, query_params_));
    EXPECT_EQ(query_params_["q"], "中文");
}

/**
 * @brief 测试边界条件的URL解码 - Test edge cases in URL decoding
 */
TEST_F(RouterOptimizationIntegrationTest, UrlDecoding_EdgeCases)
{
    auto handler = std::make_shared<TestHandler>(1, "edge_test");
    router_.add_route(HttpMethod::GET, "/api/test", handler);

    std::shared_ptr<TestHandler> found_handler;

    // 测试不完整的百分号编码 - Test incomplete percent encoding
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/test?param=hello%2", found_handler,
                                    params_, query_params_));
    EXPECT_EQ(query_params_["param"], "hello%2"); // 应该保持原样

    query_params_.clear();
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/test?param=hello%", found_handler,
                                    params_, query_params_));
    EXPECT_EQ(query_params_["param"], "hello%"); // 应该保持原样

    // 测试无效的十六进制字符 - Test invalid hex characters
    query_params_.clear();
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/test?param=hello%XY", found_handler,
                                    params_, query_params_));
    EXPECT_EQ(query_params_["param"], "hello%XY"); // 应该保持原样

    // 测试空参数 - Test empty parameters
    query_params_.clear();
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/test?empty=&param=value", found_handler,
                                    params_, query_params_));
    EXPECT_EQ(query_params_["empty"], "");
    EXPECT_EQ(query_params_["param"], "value");
}

// ============================================================================
// 性能基准测试 - Performance Benchmark Tests
// ============================================================================

/**
 * @brief 性能基准测试 - Performance benchmark test
 */
TEST_F(RouterOptimizationIntegrationTest, Performance_LargeRouteSet)
{
    const int num_routes = 1000;
    const int lookup_iterations = 1000;

    // 添加大量路由以模拟真实场景 - Add many routes to simulate real scenario
    for (int i = 0; i < num_routes; ++i) {
        auto handler = std::make_shared<TestHandler>(i, "handler_" + std::to_string(i));

        // 静态路由 - Static routes
        router_.add_route(HttpMethod::GET, "/api/static/route" + std::to_string(i), handler);

        // 参数化路由 - Parameterized routes
        router_.add_route(HttpMethod::GET, "/api/users/:id/action" + std::to_string(i), handler);

        // 通配符路由 - Wildcard routes
        router_.add_route(HttpMethod::GET, "/api/files/" + std::to_string(i) + "/*", handler);
    }

    std::shared_ptr<TestHandler> found_handler;

    // 测试静态路由查找性能 - Test static route lookup performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < lookup_iterations; ++i) {
        router_.find_route(HttpMethod::GET, "/api/static/route500", found_handler, params_,
                           query_params_);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto static_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 测试参数化路由查找性能 - Test parameterized route lookup performance
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < lookup_iterations; ++i) {
        router_.find_route(HttpMethod::GET, "/api/users/12345/action500", found_handler, params_,
                           query_params_);
    }
    end = std::chrono::high_resolution_clock::now();
    auto param_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 测试通配符路由查找性能 - Test wildcard route lookup performance
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < lookup_iterations; ++i) {
        router_.find_route(HttpMethod::GET, "/api/files/500/docs/readme.txt", found_handler,
                           params_, query_params_);
    }
    end = std::chrono::high_resolution_clock::now();
    auto wildcard_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 输出性能结果 - Output performance results
    std::cout << "\n=== Performance Benchmark with " << num_routes << " routes ===" << std::endl;
    std::cout << "Static route lookup: "
              << static_duration.count() / static_cast<double>(lookup_iterations)
              << " μs per lookup" << std::endl;
    std::cout << "Parameterized route lookup: "
              << param_duration.count() / static_cast<double>(lookup_iterations) << " μs per lookup"
              << std::endl;
    std::cout << "Wildcard route lookup: "
              << wildcard_duration.count() / static_cast<double>(lookup_iterations)
              << " μs per lookup" << std::endl;

    // 性能要求验证 - Performance requirement verification
    EXPECT_LT(static_duration.count() / static_cast<double>(lookup_iterations), 5.0);     // < 5μs
    EXPECT_LT(param_duration.count() / static_cast<double>(lookup_iterations), 50.0);     // < 50μs
    EXPECT_LT(wildcard_duration.count() / static_cast<double>(lookup_iterations), 100.0); // < 100μs
}

/**
 * @brief 缓存性能测试 - Cache performance test
 */
TEST_F(RouterOptimizationIntegrationTest, Performance_CacheEfficiency)
{
    // 添加一些路由 - Add some routes
    auto handler = std::make_shared<TestHandler>(1, "cache_test");
    router_.add_route(HttpMethod::GET, "/api/cached/:id", handler);

    std::shared_ptr<TestHandler> found_handler;
    const int iterations = 10000;

    // 首次查找（填充缓存）- First lookup (populate cache)
    router_.find_route(HttpMethod::GET, "/api/cached/123", found_handler, params_, query_params_);

    // 测试缓存命中性能 - Test cache hit performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        router_.find_route(HttpMethod::GET, "/api/cached/123", found_handler, params_,
                           query_params_);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto cached_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    // 清除缓存并测试冷查找性能 - Clear cache and test cold lookup performance
    router_.clear_cache();
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        router_.find_route(HttpMethod::GET, "/api/cached/123", found_handler, params_,
                           query_params_);
        router_.clear_cache(); // 每次都清除缓存以模拟冷查找
    }
    end = std::chrono::high_resolution_clock::now();
    auto cold_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    std::cout << "\n=== Cache Performance ===" << std::endl;
    std::cout << "Cached lookup: " << cached_duration.count() / static_cast<double>(iterations)
              << " ns per lookup" << std::endl;
    std::cout << "Cold lookup: " << cold_duration.count() / static_cast<double>(iterations)
              << " ns per lookup" << std::endl;

    // 缓存查找应该明显更快 - Cached lookup should be significantly faster
    double speedup = static_cast<double>(cold_duration.count()) / cached_duration.count();
    std::cout << "Cache speedup: " << speedup << "x faster" << std::endl;

    EXPECT_GT(speedup, 2.0); // 缓存至少应该快2倍 - Cache should be at least 2x faster
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
        auto handler = std::make_shared<TestHandler>(i, "thread_test_" + std::to_string(i));
        router_.add_route(HttpMethod::GET, "/api/thread/test" + std::to_string(i), handler);
        router_.add_route(HttpMethod::GET, "/api/thread/param/:id/test" + std::to_string(i),
                          handler);
    }

    const int num_threads = 8;
    const int operations_per_thread = 1000;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};

    // 创建多个线程同时进行路由查找 - Create multiple threads for concurrent route lookup
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            std::shared_ptr<TestHandler> local_handler;
            std::map<std::string, std::string> local_params;
            std::map<std::string, std::string> local_query_params;

            for (int i = 0; i < operations_per_thread; ++i) {
                // 测试静态路由 - Test static routes
                std::string static_path = "/api/thread/test" + std::to_string(i % 50);
                if (router_.find_route(HttpMethod::GET, static_path, local_handler, local_params,
                                       local_query_params) == 0) {
                    success_count++;
                } else {
                    error_count++;
                }

                // 测试参数化路由 - Test parameterized routes
                std::string param_path = "/api/thread/param/" + std::to_string(t * 1000 + i) +
                                         "/test" + std::to_string(i % 50);
                local_params.clear();
                local_query_params.clear();
                if (router_.find_route(HttpMethod::GET, param_path, local_handler, local_params,
                                       local_query_params) == 0) {
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
    auto handler = std::make_shared<TestHandler>(1, "large_data");

    // 添加路由处理大路径 - Add routes for large paths
    router_.add_route(HttpMethod::GET, "/api/data/:id", handler);
    router_.add_route(HttpMethod::GET, "/api/files/*", handler);

    std::shared_ptr<TestHandler> found_handler;

    // 测试长路径参数 - Test long path parameters
    std::string long_id(1000, 'a'); // 1000个'a'字符
    std::string long_path = "/api/data/" + long_id;
    EXPECT_EQ(
        0, router_.find_route(HttpMethod::GET, long_path, found_handler, params_, query_params_));
    EXPECT_EQ(params_["id"], long_id);

    // 测试长通配符路径 - Test long wildcard path
    params_.clear();
    std::string long_wildcard_path = "/api/files/";
    for (int i = 0; i < 100; ++i) {
        long_wildcard_path += "very_long_directory_name_" + std::to_string(i) + "/";
    }
    long_wildcard_path += "file.txt";

    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, long_wildcard_path, found_handler, params_,
                                    query_params_));
    EXPECT_GT(params_["*"].length(), 1000);

    // 测试大量查询参数 - Test many query parameters
    query_params_.clear();
    std::string query_path = "/api/data/test?";
    for (int i = 0; i < 100; ++i) {
        if (i > 0)
            query_path += "&";
        query_path += "param" + std::to_string(i) + "=value" + std::to_string(i);
    }

    EXPECT_EQ(
        0, router_.find_route(HttpMethod::GET, query_path, found_handler, params_, query_params_));
    EXPECT_EQ(query_params_.size(), 100);
    EXPECT_EQ(query_params_["param50"], "value50");
}

/**
 * @brief 测试路径边界条件 - Test path boundary conditions
 */
TEST_F(RouterOptimizationIntegrationTest, Boundary_PathEdgeCases)
{
    auto handler = std::make_shared<TestHandler>(1, "boundary_test");

    // 添加各种边界情况的路由 - Add routes for various boundary cases
    router_.add_route(HttpMethod::GET, "/", handler);
    router_.add_route(HttpMethod::GET, "/api", handler);
    router_.add_route(HttpMethod::GET, "/api/:param", handler);
    router_.add_route(HttpMethod::GET, "/static/*", handler);

    std::shared_ptr<TestHandler> found_handler;

    // 测试根路径 - Test root path
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/", found_handler, params_, query_params_));

    // 测试连续斜杠 - Test consecutive slashes
    EXPECT_EQ(0,
              router_.find_route(HttpMethod::GET, "//api", found_handler, params_, query_params_));
    EXPECT_EQ(0,
              router_.find_route(HttpMethod::GET, "/api//", found_handler, params_, query_params_));

    // 测试特殊字符参数 - Test special character parameters
    params_.clear();
    EXPECT_EQ(0, router_.find_route(HttpMethod::GET, "/api/user%20name", found_handler, params_,
                                    query_params_));
    EXPECT_EQ(params_["param"], "user name");

    // 测试空通配符 - Test empty wildcard
    params_.clear();
    EXPECT_EQ(
        0, router_.find_route(HttpMethod::GET, "/static/", found_handler, params_, query_params_));
    EXPECT_EQ(params_["*"], "");
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
    auto static_handler = std::make_shared<TestHandler>(1, "static");
    auto param_handler = std::make_shared<TestHandler>(2, "param");
    auto wildcard_handler = std::make_shared<TestHandler>(3, "wildcard");
    auto complex_handler = std::make_shared<TestHandler>(4, "complex");

    // 静态路由 - Static routes
    router_.add_route(HttpMethod::GET, "/api/health", static_handler);
    router_.add_route(HttpMethod::POST, "/api/login", static_handler);

    // 参数化路由 - Parameterized routes
    router_.add_route(HttpMethod::GET, "/api/users/:id", param_handler);
    router_.add_route(HttpMethod::PUT, "/api/users/:id/profile", param_handler);

    // 通配符路由 - Wildcard routes
    router_.add_route(HttpMethod::GET, "/static/*", wildcard_handler);
    router_.add_route(HttpMethod::GET, "/files/:type/*", wildcard_handler);

    // 复杂路由 - Complex routes
    router_.add_route(HttpMethod::GET, "/api/:version/users/:id/posts/:post_id", complex_handler);

    std::shared_ptr<TestHandler> found_handler;

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

    for (const auto &[path, method, expected_id, expected_params, expected_query] : test_cases) {
        params_.clear();
        query_params_.clear();

        EXPECT_EQ(0, router_.find_route(method, path, found_handler, params_, query_params_))
            << "Failed to find route for path: " << path;

        EXPECT_EQ(found_handler->id(), expected_id) << "Wrong handler for path: " << path;

        for (const auto &[key, value] : expected_params) {
            EXPECT_EQ(params_[key], value)
                << "Parameter mismatch for path: " << path << ", key: " << key;
        }

        for (const auto &[key, value] : expected_query) {
            EXPECT_EQ(query_params_[key], value)
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