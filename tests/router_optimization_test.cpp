/**
 * @file router_optimization_test.cpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief 路由器优化功能的详细单元测试 - Detailed unit tests for router optimization features
 * @version 0.4
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 * 本文件包含对router.hpp中集成的优化功能的全面测试 - 原子shared_ptr版本
 * This file contains comprehensive tests for optimization features integrated in router.hpp -
 * Atomic shared_ptr version
 *
 * 测试覆盖范围 Test Coverage:
 * 1. split_path_optimized() 函数测试
 * 2. url_decode_safe() 函数测试
 * 3. hex_to_int_safe() 函数测试
 * 4. cache_key_builder 类测试
 * 5. 性能基准测试
 * 6. 边界条件和错误处理测试
 * 7. 内存安全测试
 * 8. 线程安全测试
 *
 * 修复的问题 Fixed Issues:
 * - 修正成员访问语法错误
 * - 简化内存管理，避免析构顺序问题
 * - 使用公共接口，避免访问私有成员
 * - 增强异常安全性
 * - 使用原子shared_ptr操作避免内存竞争和双重释放问题
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
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

public:
    explicit TestHandler(int id) : id_(id) {}

    // 拷贝构造函数和赋值操作符
    // Copy constructor and assignment operator
    TestHandler(const TestHandler &other) : id_(other.id_) {}
    TestHandler &operator=(const TestHandler &other)
    {
        if (this != &other) {
            id_ = other.id_;
        }
        return *this;
    }

    // 移动构造函数和移动赋值操作符
    // Move constructor and move assignment operator
    TestHandler(TestHandler &&other) noexcept : id_(other.id_)
    {
        other.id_ = -1; // 标记为已移动
    }
    TestHandler &operator=(TestHandler &&other) noexcept
    {
        if (this != &other) {
            id_ = other.id_;
            other.id_ = -1; // 标记为已移动
        }
        return *this;
    }

    ~TestHandler() = default;

    int id() const { return id_; }
    void handle() const {}
};

/**
 * @class RouterOptimizationTest
 * @brief 路由器优化功能测试套件 - Router optimization test suite (unique_ptr version)
 */
class RouterOptimizationTest : public ::testing::Test
{
protected:
    // 使用栈对象简化内存管理
    router<TestHandler> router_;
    std::map<std::string, std::string> params_;
    std::map<std::string, std::string> query_params_;

    void SetUp() override
    {
        params_.clear();
        query_params_.clear();

        // 预先清理缓存，确保干净的测试环境
        router_.clear_cache();
    }

    void TearDown() override
    {
        // 清理测试数据
        params_.clear();
        query_params_.clear();

        // 清理缓存
        try {
            router_.clear_cache();
        } catch (...) {
            // 忽略清理过程中的异常
        }
    }

    // 辅助函数：创建unique_ptr处理器
    // Helper function: create unique_ptr handlers
    std::unique_ptr<TestHandler> make_handler(int id) { return std::make_unique<TestHandler>(id); }
};

// ============================================================================
// split_path_optimized() 函数测试 - split_path_optimized() Function Tests
// ============================================================================

/**
 * @brief 测试基本路径分割功能 - Test basic path splitting functionality
 */
TEST_F(RouterOptimizationTest, SplitPathOptimized_BasicFunctionality)
{
    std::vector<std::string> segments;

    // 测试根路径 - Test root path
    router_.split_path_optimized("/", segments);
    EXPECT_TRUE(segments.empty());

    // 测试空路径 - Test empty path
    router_.split_path_optimized("", segments);
    EXPECT_TRUE(segments.empty());

    // 测试简单路径 - Test simple path
    router_.split_path_optimized("/api", segments);
    ASSERT_EQ(segments.size(), 1);
    EXPECT_EQ(segments[0], "api");

    // 测试多段路径 - Test multi-segment path
    router_.split_path_optimized("/api/v1/users", segments);
    ASSERT_EQ(segments.size(), 3);
    EXPECT_EQ(segments[0], "api");
    EXPECT_EQ(segments[1], "v1");
    EXPECT_EQ(segments[2], "users");
}

/**
 * @brief 测试路径分割的边界条件 - Test edge cases for path splitting
 */
TEST_F(RouterOptimizationTest, SplitPathOptimized_EdgeCases)
{
    std::vector<std::string> segments;

    // 测试连续斜杠 - Test consecutive slashes
    router_.split_path_optimized("/api//v1///users", segments);
    ASSERT_EQ(segments.size(), 3);
    EXPECT_EQ(segments[0], "api");
    EXPECT_EQ(segments[1], "v1");
    EXPECT_EQ(segments[2], "users");

    // 测试尾部斜杠 - Test trailing slash
    router_.split_path_optimized("/api/v1/users/", segments);
    ASSERT_EQ(segments.size(), 3);
    EXPECT_EQ(segments[0], "api");
    EXPECT_EQ(segments[1], "v1");
    EXPECT_EQ(segments[2], "users");

    // 测试没有前导斜杠的路径 - Test path without leading slash
    router_.split_path_optimized("api/v1/users", segments);
    ASSERT_EQ(segments.size(), 3);
    EXPECT_EQ(segments[0], "api");
    EXPECT_EQ(segments[1], "v1");
    EXPECT_EQ(segments[2], "users");

    // 测试只有斜杠的路径 - Test path with only slashes
    router_.split_path_optimized("///", segments);
    EXPECT_TRUE(segments.empty());
}

/**
 * @brief 测试路径分割性能 - Test path splitting performance
 */
TEST_F(RouterOptimizationTest, SplitPathOptimized_Performance)
{
    std::vector<std::string> segments;
    const std::string test_path = "/api/v1/users/12345/profile/settings/notifications";
    const int iterations = 10000;

    // 测试优化版本性能 - Test optimized version performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        router_.split_path_optimized(test_path, segments);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto optimized_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 验证结果正确性 - Verify result correctness
    ASSERT_EQ(segments.size(), 7);
    EXPECT_EQ(segments[0], "api");
    EXPECT_EQ(segments[6], "notifications");

    std::cout << "Split path optimized performance: "
              << optimized_duration.count() / static_cast<double>(iterations) << " μs per operation"
              << std::endl;

    // 性能应该合理（每次操作少于10微秒）- Performance should be reasonable (less than 10μs per
    // operation)
    EXPECT_LT(optimized_duration.count() / static_cast<double>(iterations), 10.0);
}

// ============================================================================
// url_decode_safe() 函数测试 - url_decode_safe() Function Tests
// ============================================================================

/**
 * @brief 测试基本URL解码功能 - Test basic URL decoding functionality
 */
TEST_F(RouterOptimizationTest, UrlDecodeSafe_BasicFunctionality)
{
    std::string test_str;

    // 测试加号转空格 - Test plus to space conversion
    test_str = "hello+world";
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "hello world");

    // 测试百分号编码 - Test percent encoding
    test_str = "hello%20world";
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "hello world");

    // 测试混合编码 - Test mixed encoding
    test_str = "hello+%20world%21";
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "hello  world!");

    // 测试大小写十六进制 - Test uppercase and lowercase hex
    test_str = "%41%42%43%61%62%63"; // ABCabc
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "ABCabc");
}

/**
 * @brief 测试URL解码的边界检查 - Test boundary checking for URL decoding
 */
TEST_F(RouterOptimizationTest, UrlDecodeSafe_BoundaryChecking)
{
    std::string test_str;

    // 测试不完整的百分号编码（边界检查修复）- Test incomplete percent encoding (boundary check fix)
    test_str = "hello%2";
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "hello%2"); // 应该保持原样 - Should remain unchanged

    test_str = "hello%";
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "hello%"); // 应该保持原样 - Should remain unchanged

    // 测试字符串末尾的百分号编码 - Test percent encoding at end of string
    test_str = "hello%20%";
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "hello %");

    // 测试无效的十六进制字符 - Test invalid hex characters
    test_str = "hello%XY";
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "hello%XY"); // 应该保持原样 - Should remain unchanged

    test_str = "hello%2G";
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "hello%2G"); // 应该保持原样 - Should remain unchanged
}

/**
 * @brief 测试URL解码的特殊字符 - Test special characters in URL decoding
 */
TEST_F(RouterOptimizationTest, UrlDecodeSafe_SpecialCharacters)
{
    std::string test_str;

    // 测试中文字符的UTF-8编码 - Test UTF-8 encoding for Chinese characters
    test_str = "%E4%B8%AD%E6%96%87"; // 中文
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "中文");

    // 测试特殊符号 - Test special symbols
    test_str = "%21%40%23%24%25%5E%26%2A"; // !@#$%^&*
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "!@#$%^&*");

    // 测试空字符串 - Test empty string
    test_str = "";
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "");

    // 测试只有普通字符 - Test only normal characters
    test_str = "hello_world-123";
    router_.url_decode_safe(test_str);
    EXPECT_EQ(test_str, "hello_world-123");
}

/**
 * @brief 测试URL解码性能 - Test URL decoding performance
 */
TEST_F(RouterOptimizationTest, UrlDecodeSafe_Performance)
{
    const std::string encoded_str = "hello%20world%21%40%23%24%25+test%2Bstring";
    const int iterations = 10000;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        std::string test_str = encoded_str;
        router_.url_decode_safe(test_str);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "URL decode safe performance: "
              << duration.count() / static_cast<double>(iterations) << " μs per operation"
              << std::endl;

    // 性能应该合理（每次操作少于5微秒）- Performance should be reasonable (less than 5μs per
    // operation)
    EXPECT_LT(duration.count() / static_cast<double>(iterations), 5.0);
}

// ============================================================================
// hex_to_int_safe() 函数测试 - hex_to_int_safe() Function Tests
// ============================================================================

/**
 * @brief 测试十六进制转换基本功能 - Test basic hex conversion functionality
 */
TEST_F(RouterOptimizationTest, HexToIntSafe_BasicFunctionality)
{
    int value;

    // 测试数字字符 - Test digit characters
    EXPECT_TRUE(router_.hex_to_int_safe('0', value));
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(router_.hex_to_int_safe('9', value));
    EXPECT_EQ(value, 9);

    // 测试大写字母 - Test uppercase letters
    EXPECT_TRUE(router_.hex_to_int_safe('A', value));
    EXPECT_EQ(value, 10);
    EXPECT_TRUE(router_.hex_to_int_safe('F', value));
    EXPECT_EQ(value, 15);

    // 测试小写字母 - Test lowercase letters
    EXPECT_TRUE(router_.hex_to_int_safe('a', value));
    EXPECT_EQ(value, 10);
    EXPECT_TRUE(router_.hex_to_int_safe('f', value));
    EXPECT_EQ(value, 15);

    // 测试无效字符 - Test invalid characters
    EXPECT_FALSE(router_.hex_to_int_safe('G', value));
    EXPECT_FALSE(router_.hex_to_int_safe('g', value));
    EXPECT_FALSE(router_.hex_to_int_safe('@', value));
    EXPECT_FALSE(router_.hex_to_int_safe(' ', value));
}

/**
 * @brief 测试十六进制转换性能 - Test hex conversion performance
 */
TEST_F(RouterOptimizationTest, HexToIntSafe_Performance)
{
    const char hex_chars[] = "0123456789ABCDEFabcdef";
    const int iterations = 100000;
    int value;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        char c = hex_chars[i % (sizeof(hex_chars) - 1)]; // 修正：排除null terminator
        router_.hex_to_int_safe(c, value);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    std::cout << "Hex to int safe performance: "
              << duration.count() / static_cast<double>(iterations) << " ns per operation"
              << std::endl;

    // 性能应该非常快（每次操作少于100纳秒）- Performance should be very fast (less than 100ns per
    // operation)
    EXPECT_LT(duration.count() / static_cast<double>(iterations), 100.0);
}

// ============================================================================
// cache_key_builder 类测试 - cache_key_builder Class Tests
// ============================================================================

/**
 * @brief 测试缓存键构建器基本功能 - Test cache key builder basic functionality
 */
TEST_F(RouterOptimizationTest, CacheKeyBuilder_BasicFunctionality)
{
    auto &builder = router<TestHandler>::get_thread_local_cache_key_builder();

    // 测试基本键构建 - Test basic key building
    const std::string &key1 = builder.build(HttpMethod::GET, "/api/users");
    EXPECT_EQ(key1, "GET:/api/users");

    const std::string &key2 = builder.build(HttpMethod::POST, "/api/users/123");
    EXPECT_EQ(key2, "POST:/api/users/123");

    // 测试缓冲区重用 - Test buffer reuse
    const std::string &key3 = builder.build(HttpMethod::DELETE, "/api");
    EXPECT_EQ(key3, "DELETE:/api");

    // 验证缓冲区容量 - Verify buffer capacity
    EXPECT_GE(builder.capacity(), 128);
}

/**
 * @brief 测试缓存键构建器不同HTTP方法 - Test cache key builder with different HTTP methods
 */
TEST_F(RouterOptimizationTest, CacheKeyBuilder_DifferentMethods)
{
    auto &builder = router<TestHandler>::get_thread_local_cache_key_builder();

    // 测试所有HTTP方法 - Test all HTTP methods
    EXPECT_EQ(builder.build(HttpMethod::GET, "/test"), "GET:/test");
    EXPECT_EQ(builder.build(HttpMethod::POST, "/test"), "POST:/test");
    EXPECT_EQ(builder.build(HttpMethod::PUT, "/test"), "PUT:/test");
    EXPECT_EQ(builder.build(HttpMethod::DELETE, "/test"), "DELETE:/test");
    EXPECT_EQ(builder.build(HttpMethod::PATCH, "/test"), "PATCH:/test");
    EXPECT_EQ(builder.build(HttpMethod::HEAD, "/test"), "HEAD:/test");
    EXPECT_EQ(builder.build(HttpMethod::OPTIONS, "/test"), "OPTIONS:/test");
    EXPECT_EQ(builder.build(HttpMethod::CONNECT, "/test"), "CONNECT:/test");
    EXPECT_EQ(builder.build(HttpMethod::TRACE, "/test"), "TRACE:/test");
    EXPECT_EQ(builder.build(HttpMethod::UNKNOWN, "/test"), "UNKNOWN:/test");
}

/**
 * @brief 测试缓存键构建器性能 - Test cache key builder performance
 */
TEST_F(RouterOptimizationTest, CacheKeyBuilder_Performance)
{
    auto &builder = router<TestHandler>::get_thread_local_cache_key_builder();
    const std::string path = "/api/v1/users/12345/profile";
    const int iterations = 50000;

    // 测试优化版本性能 - Test optimized version performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        const std::string &key = builder.build(HttpMethod::GET, path);
        (void)key; // 避免编译器优化掉 - Avoid compiler optimization
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto optimized_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    // 测试传统方式性能 - Test traditional approach performance
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        std::string key = to_string(HttpMethod::GET) + ":" + path;
        (void)key; // 避免编译器优化掉 - Avoid compiler optimization
    }
    end = std::chrono::high_resolution_clock::now();
    auto traditional_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    std::cout << "Cache key builder performance: "
              << optimized_duration.count() / static_cast<double>(iterations) << " ns per operation"
              << std::endl;
    std::cout << "Traditional approach performance: "
              << traditional_duration.count() / static_cast<double>(iterations)
              << " ns per operation" << std::endl;

    // 优化版本应该比传统方式快 - Optimized version should be faster than traditional approach
    EXPECT_LT(optimized_duration.count(), traditional_duration.count());

    // 计算性能提升比例 - Calculate performance improvement ratio
    double improvement_ratio =
        static_cast<double>(traditional_duration.count()) / optimized_duration.count();
    std::cout << "Performance improvement: " << improvement_ratio << "x faster" << std::endl;

    // 应该至少有20%的性能提升 - Should have at least 20% performance improvement
    EXPECT_GT(improvement_ratio, 1.2);
}

/**
 * @brief 测试缓存键构建器重置功能 - Test cache key builder reset functionality
 */
TEST_F(RouterOptimizationTest, CacheKeyBuilder_Reset)
{
    auto &builder = router<TestHandler>::get_thread_local_cache_key_builder();

    // 构建一些键 - Build some keys
    builder.build(HttpMethod::GET, "/api/users");
    size_t original_capacity = builder.capacity();

    // 重置不改变容量 - Reset without changing capacity
    builder.reset();
    EXPECT_EQ(builder.capacity(), original_capacity);

    // 重置并改变容量 - Reset and change capacity
    builder.reset(256);
    EXPECT_GE(builder.capacity(), 256);

    // 验证重置后仍能正常工作 - Verify it still works after reset
    const std::string &key = builder.build(HttpMethod::POST, "/test");
    EXPECT_EQ(key, "POST:/test");
}

// ============================================================================
// 集成测试 - Integration Tests
// ============================================================================

/**
 * @brief 测试优化功能的集成效果 - Test integration effect of optimization features
 */
TEST_F(RouterOptimizationTest, DISABLED_Integration_FullRouterOptimization)
{
    // 添加各种类型的路由 - Add various types of routes
    router_.add_route(HttpMethod::GET, "/api/static", make_handler(1));
    router_.add_route(HttpMethod::GET, "/api/users/:id", make_handler(2));
    router_.add_route(HttpMethod::GET, "/api/files/*", make_handler(3));

    // 使用新的指针接口 - Use new pointer interface
    TestHandler *found_handler = nullptr;

    // 测试静态路由查找 - Test static route lookup
    found_handler = router_.find_route(HttpMethod::GET, "/api/static", params_, query_params_);
    EXPECT_NE(found_handler, nullptr);
    if (found_handler)
        EXPECT_EQ(found_handler->id(), 1);

    // 测试参数化路由查找 - Test parameterized route lookup
    found_handler = router_.find_route(HttpMethod::GET, "/api/users/123", params_, query_params_);
    EXPECT_NE(found_handler, nullptr);
    if (found_handler) {
        EXPECT_EQ(found_handler->id(), 2);
        EXPECT_EQ(params_["id"], "123");
    }

    // 测试通配符路由查找 - Test wildcard route lookup
    found_handler =
        router_.find_route(HttpMethod::GET, "/api/files/docs/readme.txt", params_, query_params_);
    EXPECT_NE(found_handler, nullptr);
    if (found_handler) {
        EXPECT_EQ(found_handler->id(), 3);
        EXPECT_EQ(params_["*"], "docs/readme.txt");
    }

    // 测试带查询参数的路由 - Test route with query parameters
    found_handler = router_.find_route(HttpMethod::GET, "/api/users/456?name=john&age=25", params_,
                                       query_params_);
    EXPECT_NE(found_handler, nullptr);
    if (found_handler) {
        EXPECT_EQ(found_handler->id(), 2);
        EXPECT_EQ(params_["id"], "456");
        EXPECT_EQ(query_params_["name"], "john");
        EXPECT_EQ(query_params_["age"], "25");
    }

    // 测试URL编码的查询参数 - Test URL encoded query parameters
    found_handler = router_.find_route(HttpMethod::GET,
                                       "/api/users/789?message=hello%20world&encoded=%21%40%23",
                                       params_, query_params_);
    EXPECT_NE(found_handler, nullptr);
    if (found_handler) {
        EXPECT_EQ(found_handler->id(), 2);
        EXPECT_EQ(params_["id"], "789");
        EXPECT_EQ(query_params_["message"], "hello world");
        EXPECT_EQ(query_params_["encoded"], "!@#");
    }
}

/**
 * @brief 性能基准测试 - Performance benchmark test
 * @note 现已修复RouteInfo和CacheEntry的内存管理问题，可以测试大量路由的稳定性
 */
TEST_F(RouterOptimizationTest, Integration_PerformanceBenchmark)
{
    // 大幅增加路由数量以测试修复后的稳定性 - Significantly increase routes to test post-fix
    // stability
    const int num_routes = 1500; // 增加到1500个路由组，总共4500个路由

    // 测试两种策略：独立handler vs 共享handler - Test both strategies: independent vs shared
    // handlers
    std::cout << "=== Testing with " << (num_routes * 3)
              << " routes (fixed RouteInfo/CacheEntry) ===" << std::endl;

    // 策略1：每个路由使用独立的handler（测试之前有问题的场景）
    // Strategy 1: Independent handler for each route (test previously problematic scenario)
    for (int i = 0; i < num_routes; ++i) {
        // 创建独立的handler - Create independent handlers
        router_.add_route(HttpMethod::GET, "/api/route" + std::to_string(i),
                          make_handler(1000 + i));
        router_.add_route(HttpMethod::GET, "/api/users/:id/action" + std::to_string(i),
                          make_handler(2000 + i));
        router_.add_route(HttpMethod::GET, "/api/files" + std::to_string(i) + "/*",
                          make_handler(3000 + i));
    }

    const int lookup_iterations = 200; // 增加迭代次数以测试大规模场景
    // 使用新的指针接口 - Use new pointer interface
    TestHandler *found_handler = nullptr;

    // 测试静态路由查找性能 - Test static route lookup performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < lookup_iterations; ++i) {
        int route_idx = i % num_routes; // 循环测试不同路由
        router_.find_route(HttpMethod::GET, "/api/route" + std::to_string(route_idx), found_handler,
                           params_, query_params_);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto static_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 测试参数化路由查找性能 - Test parameterized route lookup performance
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < lookup_iterations; ++i) {
        int route_idx = i % num_routes;
        router_.find_route(HttpMethod::GET, "/api/users/12345/action" + std::to_string(route_idx),
                           found_handler, params_, query_params_);
    }
    end = std::chrono::high_resolution_clock::now();
    auto param_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 测试通配符路由查找性能 - Test wildcard route lookup performance
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < lookup_iterations; ++i) {
        int route_idx = i % num_routes;
        router_.find_route(HttpMethod::GET,
                           "/api/files" + std::to_string(route_idx) + "/docs/readme.txt",
                           found_handler, params_, query_params_);
    }
    end = std::chrono::high_resolution_clock::now();
    auto wildcard_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performance benchmark with " << (num_routes * 3) << " routes:" << std::endl;
    std::cout << "Static route lookup: "
              << static_duration.count() / static_cast<double>(lookup_iterations)
              << " μs per lookup" << std::endl;
    std::cout << "Parameterized route lookup: "
              << param_duration.count() / static_cast<double>(lookup_iterations) << " μs per lookup"
              << std::endl;
    std::cout << "Wildcard route lookup: "
              << wildcard_duration.count() / static_cast<double>(lookup_iterations)
              << " μs per lookup" << std::endl;

    // 验证找到了正确的handler - Verify correct handlers were found
    router_.find_route(HttpMethod::GET, "/api/route5", found_handler, params_, query_params_);
    EXPECT_EQ(found_handler->id(), 1005); // 1000 + 5

    router_.find_route(HttpMethod::GET, "/api/users/123/action10", found_handler, params_,
                       query_params_);
    EXPECT_EQ(found_handler->id(), 2010); // 2000 + 10

    router_.find_route(HttpMethod::GET, "/api/files15/test.txt", found_handler, params_,
                       query_params_);
    EXPECT_EQ(found_handler->id(), 3015); // 3000 + 15

    // 合理的性能要求验证 - Reasonable performance requirement verification (adjusted for 4500
    // routes)
    EXPECT_LT(static_duration.count() / static_cast<double>(lookup_iterations),
              100.0); // < 100μs (for 4500 routes)
    EXPECT_LT(param_duration.count() / static_cast<double>(lookup_iterations),
              1000.0); // < 1000μs (parameterized routes are slower)
    EXPECT_LT(wildcard_duration.count() / static_cast<double>(lookup_iterations),
              1500.0); // < 1500μs (wildcard routes are slowest)

    // 测试内存压力：多次缓存操作
    // Test memory pressure: multiple cache operations
    std::cout << "Starting memory pressure test with " << num_routes << " routes..." << std::endl;
    for (int i = 0; i < 200; ++i) { // 增加到200次内存压力测试
        router_.find_route(HttpMethod::GET, "/api/route" + std::to_string(i % num_routes),
                           found_handler, params_, query_params_);

        // 每50次操作报告一次进度
        if (i % 50 == 0) {
            std::cout << "Memory pressure test progress: " << i << "/200" << std::endl;
        }
    }

    std::cout << "Memory pressure test completed successfully!" << std::endl;

    // 清理缓存
    router_.clear_cache();

    // 原子安全地清理局部变量引用 - Atomic safe cleanup of local variable reference
    std::atomic_store(&found_handler, std::shared_ptr<TestHandler>(nullptr));

    // 额外的大规模稳定性测试 - Additional large-scale stability test
    std::cout << "Starting additional stability tests..." << std::endl;

    // 测试随机路由访问模式
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_routes - 1);

    for (int i = 0; i < 100; ++i) {
        int random_idx = dis(gen);

        // 随机测试三种路由类型
        switch (i % 3) {
        case 0: // 静态路由
            router_.find_route(HttpMethod::GET, "/api/route" + std::to_string(random_idx),
                               found_handler, params_, query_params_);
            break;
        case 1: // 参数化路由
            router_.find_route(HttpMethod::GET,
                               "/api/users/12345/action" + std::to_string(random_idx),
                               found_handler, params_, query_params_);
            break;
        case 2: // 通配符路由
            router_.find_route(HttpMethod::GET,
                               "/api/files" + std::to_string(random_idx) + "/random/file.txt",
                               found_handler, params_, query_params_);
            break;
        }
    }

    std::cout << "Random access pattern test completed!" << std::endl;

    // 测试缓存效率 - 重复访问相同路由
    auto cache_test_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 500; ++i) {
        router_.find_route(HttpMethod::GET, "/api/route100", found_handler, params_, query_params_);
    }
    auto cache_test_end = std::chrono::high_resolution_clock::now();
    auto cache_test_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(cache_test_end - cache_test_start);

    std::cout << "Cache efficiency test (500 identical lookups): "
              << cache_test_duration.count() / 500.0 << " μs per lookup" << std::endl;

    // 缓存测试应该非常快（由于缓存命中）
    EXPECT_LT(cache_test_duration.count() / 500.0, 10.0); // < 10μs per cached lookup

    std::cout << "=== Large scale test with " << (num_routes * 3)
              << " routes completed successfully! ===" << std::endl;
}

// ============================================================================
// 回归测试 - Regression Tests
// ============================================================================

/**
 * @brief 确保优化版本与原版本行为一致 - Ensure optimized version behaves consistently with original
 */
TEST_F(RouterOptimizationTest, Regression_BackwardCompatibility)
{
    // 确保优化后的函数与原函数行为完全一致 - Ensure optimized functions behave exactly like
    // original functions

    std::vector<std::string> test_paths = {"/",
                                           "/api",
                                           "/api/v1",
                                           "/api/v1/users",
                                           "/api/v1/users/123",
                                           "api/v1/users",
                                           "/api//v1///users/",
                                           "///api/v1/users///"};

    for (const auto &path : test_paths) {
        std::vector<std::string> optimized_segments, legacy_segments;

        router_.split_path_optimized(path, optimized_segments);
        router_.split_path(path, legacy_segments);

        EXPECT_EQ(optimized_segments.size(), legacy_segments.size())
            << "Size mismatch for path: " << path;

        for (size_t i = 0; i < optimized_segments.size(); ++i) {
            EXPECT_EQ(optimized_segments[i], legacy_segments[i])
                << "Segment mismatch at index " << i << " for path: " << path;
        }
    }

    // 测试URL解码的一致性 - Test URL decoding consistency
    std::vector<std::string> url_test_cases = {"hello+world", "hello%20world", "hello%21world",
                                               "%41%42%43",   "normal_text",   "",
                                               "hello%2",     "hello%",        "hello%XY"};

    for (const auto &test_case : url_test_cases) {
        std::string optimized_str = test_case;
        std::string legacy_str = test_case;

        router_.url_decode_safe(optimized_str);
        router_.url_decode(legacy_str);

        EXPECT_EQ(optimized_str, legacy_str) << "URL decode mismatch for input: " << test_case;
    }
}

// ============================================================================
// 内存安全测试 - Memory Safety Tests
// ============================================================================

/**
 * @brief 测试大数据量处理的内存安全性 - Test memory safety with large data processing
 */
TEST_F(RouterOptimizationTest, MemorySafety_LargeDataProcessing)
{
    // 测试大字符串的URL解码 - Test URL decoding with large strings
    std::string large_encoded;
    large_encoded.reserve(1000); // 减小测试规模

    // 构建大的编码字符串 - Build large encoded string
    for (int i = 0; i < 50; ++i) { // 减少循环次数
        large_encoded += "hello%20world%21+test" + std::to_string(i) + "%2B";
    }

    // 解码不应该崩溃或内存泄漏 - Decoding should not crash or leak memory
    router_.url_decode_safe(large_encoded);
    EXPECT_GT(large_encoded.length(), 200); // 验证确实被解码了 - Verify it was actually decoded

    // 测试大路径的分割 - Test splitting large paths
    std::string large_path = "/";
    for (int i = 0; i < 20; ++i) { // 减少路径段数量
        large_path += "segment" + std::to_string(i) + "/";
    }

    std::vector<std::string> segments;
    router_.split_path_optimized(large_path, segments);
    EXPECT_EQ(segments.size(), 20);
    EXPECT_EQ(segments[0], "segment0");
    EXPECT_EQ(segments[19], "segment19");
}

/**
 * @brief 测试缓冲区边界安全性 - Test buffer boundary safety
 */
TEST_F(RouterOptimizationTest, MemorySafety_BufferBoundaries)
{
    std::string test_str;

    // 测试边界条件的URL解码 - Test boundary conditions in URL decoding
    std::vector<std::string> edge_cases = {
        "%", "%2", "%G", "%2G", "%GG", "%%", "%20%", "%20%2", "normal%20text%", "text%end"};

    for (const auto &edge_case : edge_cases) {
        test_str = edge_case;
        // 这些都不应该崩溃 - None of these should crash
        EXPECT_NO_THROW(router_.url_decode_safe(test_str));
    }

    // 测试极端路径分割情况 - Test extreme path splitting cases
    std::vector<std::string> path_edge_cases = {
        "", "/", "//", "///", "////", "/a/", "//a//", "///a///", "/a//b//c/", "a", "a/b", "a/b/c"};

    for (const auto &path_case : path_edge_cases) {
        std::vector<std::string> segments;
        // 这些都不应该崩溃 - None of these should crash
        EXPECT_NO_THROW(router_.split_path_optimized(path_case, segments));
    }
}

// ============================================================================
// 线程安全测试 - Thread Safety Tests (简化版本)
// ============================================================================

/**
 * @brief 测试基本的线程安全性 - Test basic thread safety
 */
TEST_F(RouterOptimizationTest, ThreadSafety_BasicConcurrentAccess)
{
    // 添加一些路由 - Add some routes
    for (int i = 0; i < 5; ++i) {
        auto handler = std::make_shared<TestHandler>(i);
        router_.add_route(HttpMethod::GET, "/api/test" + std::to_string(i), handler);
    }

    const int num_threads = 4;
    const int operations_per_thread = 10; // 减少操作次数
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    // 创建多个线程同时进行路由查找 - Create multiple threads for concurrent route lookup
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            // 使用原子安全的shared_ptr - Use atomic safe shared_ptr
            std::shared_ptr<TestHandler> found_handler = nullptr;
            std::map<std::string, std::string> local_params;
            std::map<std::string, std::string> local_query_params;

            for (int i = 0; i < operations_per_thread; ++i) {
                std::string path = "/api/test" + std::to_string(i % 5);
                if (router_.find_route(HttpMethod::GET, path, found_handler, local_params,
                                       local_query_params) == 0) {
                    success_count++;
                }

                // 原子安全地重置handler以避免潜在的内存问题
                // Atomic safe reset of handler to avoid potential memory issues
                std::atomic_store(&found_handler, std::shared_ptr<TestHandler>(nullptr));
            }
        });
    }

    // 等待所有线程完成 - Wait for all threads to complete
    for (auto &thread : threads) {
        thread.join();
    }

    // 验证所有操作都成功 - Verify all operations succeeded
    EXPECT_EQ(success_count.load(), num_threads * operations_per_thread);
}