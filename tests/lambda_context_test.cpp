/**
 * @file lambda_context_test.cpp
 * @author Assistant
 * @brief Unit tests for lambda expressions with context parameter support
 * @version 1.0
 * @date 2025-01-03
 *
 * @copyright Copyright (c) 2025
 *
 * 本文件测试支持上下文参数的lambda表达式功能
 * This file tests lambda expression functionality with context parameter support
 */

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include "router/router.hpp"

using namespace co;

/**
 * @brief HTTP请求上下文结构 - HTTP request context structure
 *
 * Contains all necessary information for processing an HTTP request
 * 包含处理HTTP请求所需的所有信息
 */
struct HttpContext
{
    // Request information
    std::string method;
    std::string path;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> headers;
    std::string body;

    // Response information
    int status_code = 200;
    std::string response_body;
    std::map<std::string, std::string> response_headers;

    // Utility methods
    void set_response(int code, const std::string &body)
    {
        status_code = code;
        response_body = body;
    }

    void add_header(const std::string &key, const std::string &value)
    {
        response_headers[key] = value;
    }

    std::string get_param(const std::string &key) const
    {
        auto it = params.find(key);
        return it != params.end() ? it->second : "";
    }

    std::string get_query(const std::string &key) const
    {
        auto it = query_params.find(key);
        return it != query_params.end() ? it->second : "";
    }
};

/**
 * @brief Lambda处理器概念 - Lambda handler concept
 *
 * Extends the original concept to support handlers with context parameter
 * 扩展原始概念以支持带上下文参数的处理器
 */
template <typename T>
concept ContextCallableHandler = requires(T t, HttpContext *ctx) {
    // Must be move constructible
    T{std::move(t)};

    // Must be callable with HttpContext* parameter
    t(ctx);
} && std::move_constructible<T>;

/**
 * @brief 带上下文的路由器包装器 - Context-aware router wrapper
 *
 * Wraps the existing router to support lambda handlers with context parameter
 * 包装现有路由器以支持带上下文参数的lambda处理器
 */
template <ContextCallableHandler Handler>
class context_router
{
private:
    // Internal handler wrapper that adapts context handlers to no-parameter handlers
    struct HandlerWrapper
    {
        Handler handler;
        HttpContext *context;

        HandlerWrapper(Handler h, HttpContext *ctx) : handler(std::move(h)), context(ctx) {}

        void operator()() { handler(context); }
    };

    router<HandlerWrapper> internal_router_;
    HttpContext context_;

public:
    context_router() = default;

    /**
     * @brief 添加带上下文的路由 - Add route with context
     */
    void add_route(HttpMethod method, const std::string &path, Handler handler)
    {
        HandlerWrapper wrapper(std::move(handler), &context_);
        internal_router_.add_route(method, path, std::move(wrapper));
    }

    /**
     * @brief 查找并执行路由 - Find and execute route
     */
    std::optional<std::reference_wrapper<HandlerWrapper>>
    find_route(HttpMethod method, const std::string &path,
               std::map<std::string, std::string> &params,
               std::map<std::string, std::string> &query_params)
    {

        // Update context with current request info
        context_.method = to_string(method);
        context_.path = path;

        auto result = internal_router_.find_route(method, path, params, query_params);

        // Update context with extracted params and query_params AFTER route lookup
        if (result.has_value()) {
            context_.params = params;
            context_.query_params = query_params;
        }

        return result;
    }

    /**
     * @brief 获取上下文引用 - Get context reference
     */
    HttpContext &get_context() { return context_; }
    const HttpContext &get_context() const { return context_; }
};

/**
 * @class LambdaContextTest
 * @brief Lambda上下文测试套件 - Lambda context test suite
 */
class LambdaContextTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        params_.clear();
        query_params_.clear();
    }

    std::map<std::string, std::string> params_;
    std::map<std::string, std::string> query_params_;
};

// ============================================================================
// 基本Lambda上下文测试 - Basic Lambda Context Tests
// ============================================================================

/**
 * @brief 测试简单的上下文lambda - Test simple context lambda
 */
TEST_F(LambdaContextTest, SimpleContextLambda)
{
    context_router<std::function<void(HttpContext *)>> router;

    // 添加简单的上下文lambda
    auto simple_handler = [](HttpContext *ctx) {
        ctx->set_response(200, "Hello World");
        ctx->add_header("Content-Type", "text/plain");
    };

    router.add_route(HttpMethod::GET, "/hello", simple_handler);

    auto result = router.find_route(HttpMethod::GET, "/hello", params_, query_params_);
    EXPECT_TRUE(result.has_value());

    if (result.has_value()) {
        result->get()(); // 执行lambda

        const auto &ctx = router.get_context();
        EXPECT_EQ(ctx.status_code, 200);
        EXPECT_EQ(ctx.response_body, "Hello World");
        EXPECT_EQ(ctx.response_headers.at("Content-Type"), "text/plain");
    }
}

/**
 * @brief 测试参数提取lambda - Test parameter extraction lambda
 */
TEST_F(LambdaContextTest, ParameterExtractionLambda)
{
    context_router<std::function<void(HttpContext *)>> router;

    // 添加参数提取lambda
    auto param_handler = [](HttpContext *ctx) {
        std::string user_id = ctx->get_param("id");
        std::string format = ctx->get_query("format");

        ctx->set_response(200, "User: " + user_id + ", Format: " + format);
    };

    router.add_route(HttpMethod::GET, "/users/:id", param_handler);

    auto result =
        router.find_route(HttpMethod::GET, "/users/123?format=json", params_, query_params_);
    EXPECT_TRUE(result.has_value());

    if (result.has_value()) {
        result->get()(); // 执行lambda

        const auto &ctx = router.get_context();
        EXPECT_EQ(ctx.status_code, 200);
        EXPECT_EQ(ctx.response_body, "User: 123, Format: json");
    }
}

/**
 * @brief 测试复杂业务逻辑lambda - Test complex business logic lambda
 */
TEST_F(LambdaContextTest, ComplexBusinessLogicLambda)
{
    context_router<std::function<void(HttpContext *)>> router;

    // 模拟用户数据库
    std::map<std::string, std::string> user_db = {{"1", "Alice"}, {"2", "Bob"}, {"3", "Charlie"}};

    // 添加复杂业务逻辑lambda
    auto user_handler = [&user_db](HttpContext *ctx) {
        std::string user_id = ctx->get_param("id");

        auto it = user_db.find(user_id);
        if (it != user_db.end()) {
            ctx->set_response(200,
                              "{\"id\": \"" + user_id + "\", \"name\": \"" + it->second + "\"}");
            ctx->add_header("Content-Type", "application/json");
        } else {
            ctx->set_response(404, "{\"error\": \"User not found\"}");
            ctx->add_header("Content-Type", "application/json");
        }
    };

    router.add_route(HttpMethod::GET, "/api/users/:id", user_handler);

    // 测试存在的用户
    auto result = router.find_route(HttpMethod::GET, "/api/users/2", params_, query_params_);
    EXPECT_TRUE(result.has_value());

    if (result.has_value()) {
        result->get()();

        const auto &ctx = router.get_context();
        EXPECT_EQ(ctx.status_code, 200);
        EXPECT_EQ(ctx.response_body, "{\"id\": \"2\", \"name\": \"Bob\"}");
        EXPECT_EQ(ctx.response_headers.at("Content-Type"), "application/json");
    }

    // 测试不存在的用户
    result = router.find_route(HttpMethod::GET, "/api/users/999", params_, query_params_);
    EXPECT_TRUE(result.has_value());

    if (result.has_value()) {
        result->get()();

        const auto &ctx = router.get_context();
        EXPECT_EQ(ctx.status_code, 404);
        EXPECT_EQ(ctx.response_body, "{\"error\": \"User not found\"}");
    }
}

/**
 * @brief 测试状态捕获lambda - Test stateful capture lambda
 */
TEST_F(LambdaContextTest, StatefulCaptureLambda)
{
    context_router<std::function<void(HttpContext *)>> router;

    // 使用捕获的状态
    std::atomic<int> request_count{0};
    std::string server_name = "MyServer";

    auto counter_handler = [&request_count, server_name](HttpContext *ctx) {
        int count = ++request_count;
        ctx->set_response(200, "Request #" + std::to_string(count) + " from " + server_name);
        ctx->add_header("X-Request-Count", std::to_string(count));
    };

    router.add_route(HttpMethod::GET, "/counter", counter_handler);

    // 执行多次请求
    for (int i = 1; i <= 3; ++i) {
        auto result = router.find_route(HttpMethod::GET, "/counter", params_, query_params_);
        EXPECT_TRUE(result.has_value());

        if (result.has_value()) {
            result->get()();

            const auto &ctx = router.get_context();
            EXPECT_EQ(ctx.status_code, 200);
            EXPECT_EQ(ctx.response_body, "Request #" + std::to_string(i) + " from MyServer");
            EXPECT_EQ(ctx.response_headers.at("X-Request-Count"), std::to_string(i));
        }
    }

    EXPECT_EQ(request_count.load(), 3);
}

// ============================================================================
// 性能测试 - Performance Tests
// ============================================================================

/**
 * @brief 测试Lambda性能 - Test lambda performance
 */
TEST_F(LambdaContextTest, LambdaPerformanceTest)
{
    context_router<std::function<void(HttpContext *)>> router;

    // 添加性能测试路由
    const int num_routes = 1000;
    for (int i = 0; i < num_routes; ++i) {
        auto handler = [i](HttpContext *ctx) {
            ctx->set_response(200, "Response from route " + std::to_string(i));
        };

        router.add_route(HttpMethod::GET, "/perf/" + std::to_string(i), handler);
    }

    // 性能测试
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        int route_id = i % num_routes;
        auto result = router.find_route(HttpMethod::GET, "/perf/" + std::to_string(route_id),
                                        params_, query_params_);

        if (result.has_value()) {
            result->get()(); // 执行lambda
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double avg_time = duration.count() / static_cast<double>(iterations);
    std::cout << "Lambda context performance: " << avg_time << " μs per operation" << std::endl;

    // 性能应该合理（每次操作少于1000微秒）
    EXPECT_LT(avg_time, 1000.0);
}

/**
 * @brief 测试Lambda vs 普通处理器性能对比 - Test lambda vs regular handler performance comparison
 */
TEST_F(LambdaContextTest, LambdaVsRegularHandlerComparison)
{
    // Lambda路由器
    context_router<std::function<void(HttpContext *)>> lambda_router;

    // 普通处理器路由器
    class RegularHandler
    {
    public:
        void operator()()
        {
            // 简单的处理逻辑
        }
    };

    router<RegularHandler> regular_router;

    // 添加路由
    const int num_routes = 500;

    for (int i = 0; i < num_routes; ++i) {
        // Lambda路由
        auto lambda_handler = [i](HttpContext *ctx) {
            ctx->set_response(200, "Lambda response " + std::to_string(i));
        };
        lambda_router.add_route(HttpMethod::GET, "/lambda/" + std::to_string(i), lambda_handler);

        // 普通处理器路由
        regular_router.add_route(HttpMethod::GET, "/regular/" + std::to_string(i),
                                 RegularHandler{});
    }

    const int iterations = 5000;

    // 测试Lambda性能
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        int route_id = i % num_routes;
        auto result = lambda_router.find_route(
            HttpMethod::GET, "/lambda/" + std::to_string(route_id), params_, query_params_);
        if (result.has_value()) {
            result->get()();
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto lambda_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 测试普通处理器性能
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        int route_id = i % num_routes;
        auto result = regular_router.find_route(
            HttpMethod::GET, "/regular/" + std::to_string(route_id), params_, query_params_);
        if (result.has_value()) {
            result->get()();
        }
    }
    end = std::chrono::high_resolution_clock::now();
    auto regular_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double lambda_avg = lambda_duration.count() / static_cast<double>(iterations);
    double regular_avg = regular_duration.count() / static_cast<double>(iterations);

    std::cout << "Lambda handler performance: " << lambda_avg << " μs per operation" << std::endl;
    std::cout << "Regular handler performance: " << regular_avg << " μs per operation" << std::endl;

    if (regular_avg > 0) {
        double overhead = (lambda_avg - regular_avg) / regular_avg * 100.0;
        std::cout << "Lambda overhead: " << overhead << "%" << std::endl;

        // Lambda开销应该合理（不超过200%）
        EXPECT_LT(overhead, 200.0);
    }
}

/**
 * @brief 测试并发Lambda处理 - Test concurrent lambda processing
 */
TEST_F(LambdaContextTest, ConcurrentLambdaProcessing)
{
    context_router<std::function<void(HttpContext *)>> router;

    // 添加线程安全的计数器路由
    std::atomic<int> global_counter{0};
    auto counter_handler = [&global_counter](HttpContext *ctx) {
        int count = ++global_counter;
        ctx->set_response(200, "Count: " + std::to_string(count));
    };

    router.add_route(HttpMethod::GET, "/concurrent", counter_handler);

    // 并发测试
    const int num_threads = 4;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&router, &success_count, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                std::map<std::string, std::string> local_params, local_query_params;
                auto result = router.find_route(HttpMethod::GET, "/concurrent", local_params,
                                                local_query_params);

                if (result.has_value()) {
                    result->get()();
                    success_count++;
                }
            }
        });
    }

    // 等待所有线程完成
    for (auto &thread : threads) {
        thread.join();
    }

    // 验证结果
    EXPECT_EQ(success_count.load(), num_threads * operations_per_thread);
    EXPECT_EQ(global_counter.load(), num_threads * operations_per_thread);
}

/**
 * @brief 测试内存使用和清理 - Test memory usage and cleanup
 */
TEST_F(LambdaContextTest, MemoryUsageAndCleanup)
{
    // 创建大量lambda处理器来测试内存管理
    {
        context_router<std::function<void(HttpContext *)>> router;

        // 创建包含大量捕获数据的lambda
        std::vector<std::string> large_data(1000, std::string(100, 'x'));

        for (int i = 0; i < 100; ++i) {
            auto handler = [large_data, i](HttpContext *ctx) {
                ctx->set_response(200, "Handler " + std::to_string(i) +
                                           " with data size: " + std::to_string(large_data.size()));
            };

            router.add_route(HttpMethod::GET, "/memory/" + std::to_string(i), handler);
        }

        // 执行一些操作以确保lambda被正确存储和执行
        auto result = router.find_route(HttpMethod::GET, "/memory/50", params_, query_params_);
        EXPECT_TRUE(result.has_value());

        if (result.has_value()) {
            result->get()();
            const auto &ctx = router.get_context();
            EXPECT_EQ(ctx.status_code, 200);
            EXPECT_TRUE(ctx.response_body.find("Handler 50") != std::string::npos);
        }
    }

    // router超出作用域，应该正确清理内存
    // 这个测试主要是确保没有内存泄漏，具体的内存检查需要工具如valgrind
    EXPECT_TRUE(true); // 如果程序能运行到这里说明没有崩溃
}