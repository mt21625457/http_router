#include "http_router.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

class DummyHandler
{
public:
    DummyHandler() = default;
    ~DummyHandler() = default;

    // 添加用于标识处理程序的ID
    explicit DummyHandler(int id) : id_(id) {}
    int id() const { return id_; }

private:
    int id_ = 0;
};

TEST(HttpRouterTest, AddAndFindRoute)
{
    http_router<DummyHandler> router;

    // Create some test handlers
    auto handler1 = std::make_shared<DummyHandler>();
    auto handler2 = std::make_shared<DummyHandler>();
    auto handler3 = std::make_shared<DummyHandler>();

    // Test adding routes
    router.add_route(HttpMethod::GET, "/test1", handler1);
    router.add_route(HttpMethod::GET, "/test2", handler2);
    router.add_route(HttpMethod::GET, "/test/nested/path", handler3);

    // Test finding existing routes
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/test1", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler1);

    EXPECT_EQ(router.find_route(HttpMethod::GET, "/test2", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler2);

    EXPECT_EQ(router.find_route(HttpMethod::GET, "/test/nested/path", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler3);

    // Test finding non-existent route
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/nonexistent", found_handler, params, query_params), -1);
}

TEST(RouterTest, ParameterExtraction)
{
    http_router<DummyHandler> router;
    auto handler = std::make_shared<DummyHandler>();

    // Add a route with a parameter
    router.add_route(HttpMethod::GET, "/users/:id", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test parameter extraction
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/users/123", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler);
    EXPECT_EQ(params["id"], "123");
}

TEST(RouterTest, MultipleParameters)
{
    http_router<DummyHandler> router;
    auto handler = std::make_shared<DummyHandler>();

    // Add a route with multiple parameters
    router.add_route(HttpMethod::GET, "/users/:userId/posts/:postId", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test multiple parameter extraction
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/users/123/posts/456", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler);
    EXPECT_EQ(params["userId"], "123");
    EXPECT_EQ(params["postId"], "456");
}

TEST(RouterTest, WildcardWithParameters)
{
    http_router<DummyHandler> router;
    auto handler = std::make_shared<DummyHandler>();

    // Add a route with wildcard and parameters
    router.add_route(HttpMethod::GET, "/files/:path/*", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test wildcard with parameter extraction
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/files/documents/report.pdf", found_handler, params, query_params),
              0);
    EXPECT_EQ(found_handler, handler);
    EXPECT_EQ(params["path"], "documents");
    EXPECT_EQ(params["*"], "report.pdf");
}

TEST(RouterTest, QueryParameters)
{
    http_router<DummyHandler> router;
    auto handler = std::make_shared<DummyHandler>();

    // Add a static route
    router.add_route(HttpMethod::GET, "/search", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test basic query parameter extraction
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/search?q=test&page=2", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler);
    EXPECT_EQ(query_params["q"], "test");
    EXPECT_EQ(query_params["page"], "2");

    // Clear params for next test
    params.clear();
    query_params.clear();

    // Test URL-encoded query parameters
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/search?q=hello+world&filter=category%3Dbooks", found_handler,
                                params, query_params),
              0);
    EXPECT_EQ(found_handler, handler);
    EXPECT_EQ(query_params["q"], "hello world");
    EXPECT_EQ(query_params["filter"], "category=books");
}

TEST(RouterTest, PathAndQueryParameters)
{
    http_router<DummyHandler> router;
    auto handler = std::make_shared<DummyHandler>();

    // Add a route with path parameters
    router.add_route(HttpMethod::GET, "/users/:userId/posts/:postId", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test combination of path and query parameters
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/users/123/posts/456?sort=date&order=desc", found_handler, params,
                                query_params),
              0);
    EXPECT_EQ(found_handler, handler);

    // Check path parameters
    EXPECT_EQ(params["userId"], "123");
    EXPECT_EQ(params["postId"], "456");

    // Check query parameters
    EXPECT_EQ(query_params["sort"], "date");
    EXPECT_EQ(query_params["order"], "desc");
}

TEST(RouterTest, QueryParametersWithoutValue)
{
    http_router<DummyHandler> router;
    auto handler = std::make_shared<DummyHandler>();

    // Add a static route
    router.add_route(HttpMethod::GET, "/options", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test query parameters without values
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/options?debug&verbose", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler);
    EXPECT_EQ(query_params["debug"], "");
    EXPECT_EQ(query_params["verbose"], "");

    // Test mixed query parameters with and without values
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/options?debug&level=info", found_handler, params, query_params),
              0);
    EXPECT_EQ(found_handler, handler);
    EXPECT_EQ(query_params["debug"], "");
    EXPECT_EQ(query_params["level"], "info");
}

// 测试混合路由策略 - 哈希表用于短路径，Trie树用于长路径
TEST(RouterTest, HybridRoutingStrategy)
{
    http_router<DummyHandler> router;

    // 创建带ID的处理程序以便区分
    auto short_handler = std::make_shared<DummyHandler>(1); // 应该存储在哈希表中
    auto long_handler = std::make_shared<DummyHandler>(2);  // 应该存储在Trie树中
    auto param_handler = std::make_shared<DummyHandler>(3); // 参数化路由

    // 添加各种类型的路由
    router.add_route(HttpMethod::GET, "/api", short_handler); // 短路径 -> 哈希表
    router.add_route(HttpMethod::GET, "/api/users/profiles/settings/notifications",
                     long_handler);                             // 长路径 -> Trie树
    router.add_route(HttpMethod::GET, "/products/:category/:id", param_handler); // 参数化路由

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试短路径查找 (哈希表)
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/api", found_handler, params, query_params), 0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 1);

    // 测试长路径查找 (Trie树)
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/api/users/profiles/settings/notifications", found_handler, params,
                                query_params),
              0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 2);

    // 测试参数化路由查找
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/products/electronics/12345", found_handler, params, query_params),
              0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 3);
    EXPECT_EQ(params["category"], "electronics");
    EXPECT_EQ(params["id"], "12345");
}

// 测试复杂路由注册场景
TEST(RouterTest, ComplexRoutingScenario)
{
    http_router<DummyHandler> router;

    // 添加大量具有各种特性的路由
    std::vector<std::shared_ptr<DummyHandler>> handlers;
    for (int i = 0; i < 50; i++) {
        handlers.push_back(std::make_shared<DummyHandler>(i));
    }

    // 短路径路由
    router.add_route(HttpMethod::GET, "/", handlers[0]);
    router.add_route(HttpMethod::GET, "/home", handlers[1]);
    router.add_route(HttpMethod::GET, "/about", handlers[2]);
    router.add_route(HttpMethod::GET, "/contact", handlers[3]);
    router.add_route(HttpMethod::POST, "/login", handlers[4]); // Example of POST
    router.add_route(HttpMethod::POST, "/signup", handlers[5]); // Example of POST

    // 长路径路由 (Trie树)
    router.add_route(HttpMethod::GET, "/api/v1/users/profiles/settings", handlers[10]);
    router.add_route(HttpMethod::GET, "/api/v1/users/profiles/photos", handlers[11]);
    router.add_route(HttpMethod::GET, "/api/v1/users/profiles/friends", handlers[12]);
    router.add_route(HttpMethod::GET, "/api/v1/posts/recent/comments", handlers[13]);
    router.add_route(HttpMethod::GET, "/api/v1/posts/trending/today", handlers[14]);

    // 参数化路由
    router.add_route(HttpMethod::GET, "/users/:userId", handlers[20]);
    router.add_route(HttpMethod::PUT, "/users/:userId/profile", handlers[21]); // Example of PUT
    router.add_route(HttpMethod::DELETE, "/users/:userId/posts/:postId", handlers[22]); // Example of DELETE
    router.add_route(HttpMethod::GET, "/products/:category/:productId/reviews", handlers[23]);

    // 通配符路由
    router.add_route(HttpMethod::GET, "/static/*", handlers[30]);
    router.add_route(HttpMethod::GET, "/files/:type/*", handlers[31]);


    // 测试各种路由查找
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试通配符
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/static/123", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 30);
    EXPECT_EQ(params["*"], "123");

    // 测试短路径
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/about", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 2);

    EXPECT_EQ(router.find_route(HttpMethod::POST, "/login", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 4);


    // 测试长路径
    params.clear();
    query_params.clear();
    EXPECT_EQ(
        router.find_route(HttpMethod::GET, "/api/v1/posts/trending/today", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 14);

    // 测试参数化路由
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::DELETE, "/users/42/posts/123", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 22);
    EXPECT_EQ(params["userId"], "42");
    EXPECT_EQ(params["postId"], "123");

    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::PUT, "/users/myuser/profile", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 21);
    EXPECT_EQ(params["userId"], "myuser");


    // 测试通配符路由
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/files/documents/reports/annual/2023.pdf", found_handler, params,
                                query_params),
              0);
    EXPECT_EQ(found_handler->id(), 31);
    EXPECT_EQ(params["type"], "documents");
    EXPECT_EQ(params["*"], "reports/annual/2023.pdf");

    // 测试混合参数和查询参数
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/products/electronics/12345/reviews?sort=newest&page=2",
                                found_handler, params, query_params),
              0);
    EXPECT_EQ(found_handler->id(), 23);
    EXPECT_EQ(params["category"], "electronics");
    EXPECT_EQ(params["productId"], "12345");
    EXPECT_EQ(query_params["sort"], "newest");
    EXPECT_EQ(query_params["page"], "2");
}

// 测试路由冲突处理
TEST(RouterTest, RouteConflictResolution)
{
    http_router<DummyHandler> router;

    // 创建处理程序
    auto handler1 = std::make_shared<DummyHandler>(1);
    auto handler2 = std::make_shared<DummyHandler>(2);
    auto handler3 = std::make_shared<DummyHandler>(3);

    // 添加潜在冲突的路由
    router.add_route(HttpMethod::GET, "/api/users", handler1);
    router.add_route(HttpMethod::GET, "/api/:resource", handler2); // 可能与静态路由冲突
    router.add_route(HttpMethod::GET, "/api/users/:id", handler3); // 更具体的路由

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试静态路由优先于参数化路由
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/api/users", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 1); // 应该匹配静态路由

    // 测试参数化路由匹配其他资源
    params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/api/products", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params["resource"], "products");

    // 测试更具体的参数化路由
    params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/api/users/42", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 3);
    EXPECT_EQ(params["id"], "42");
}

// 测试性能基准
TEST(RouterTest, PerformanceBenchmark)
{
    http_router<DummyHandler> router;

    // 添加大量路由以进行性能测试
    const int NUM_ROUTES = 1000;
    std::vector<std::shared_ptr<DummyHandler>> handlers;

    for (int i = 0; i < NUM_ROUTES; i++) {
        handlers.push_back(std::make_shared<DummyHandler>(i));
        HttpMethod method = static_cast<HttpMethod>(i % 5); // Cycle through a few methods
        if (method == HttpMethod::UNKNOWN) method = HttpMethod::GET; // Avoid UNKNOWN for adding

        // 添加各种类型的路由
        if (i % 5 == 0) {
            // 短路径
            router.add_route(method, "/short" + std::to_string(i), handlers[i]);
        } else if (i % 5 == 1) {
            // 长路径
            router.add_route(method, "/api/v1/users/profiles/settings/" + std::to_string(i), handlers[i]);
        } else if (i % 5 == 2) {
            // 参数化路由
            router.add_route(method, "/users/" + std::to_string(i) + "/:param", handlers[i]);
        } else if (i % 5 == 3) {
            // 带多个参数的路由
            router.add_route(method, "/products/:category/" + std::to_string(i) + "/:id", handlers[i]);
        } else {
            // 通配符路由
            router.add_route(method, "/files/" + std::to_string(i) + "/*", handlers[i]);
        }
    }

    // 执行一些路由匹配以验证性能
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试短路径查找（哈希表）
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/short100", found_handler, params, query_params), 0); // Assuming 100%5 == 0 -> GET
    EXPECT_EQ(found_handler->id(), 100);

    // 测试长路径查找（Trie树）
    params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::POST, "/api/v1/users/profiles/settings/101", found_handler, params, // Assuming 101%5 == 1 -> POST
                                query_params),
              0);
    EXPECT_EQ(found_handler->id(), 101);

    // 测试参数化路由
    params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::PUT, "/users/102/test", found_handler, params, query_params), 0); // Assuming 102%5 == 2 -> PUT
    EXPECT_EQ(found_handler->id(), 102);
    EXPECT_EQ(params["param"], "test");

    // 测试通配符路由
    params.clear();
    EXPECT_EQ(
        router.find_route(HttpMethod::PATCH, "/files/104/some/deep/path.txt", found_handler, params, query_params), 0); // Assuming 104%5 == 4 -> PATCH
    EXPECT_EQ(found_handler->id(), 104);
    EXPECT_EQ(params["*"], "some/deep/path.txt");
}

// 测试路由缓存功能
TEST(RouterTest, RouteCaching)
{
    http_router<DummyHandler> router;

    // 创建路由处理程序
    auto handler1 = std::make_shared<DummyHandler>(1);
    auto handler2 = std::make_shared<DummyHandler>(2);
    auto handler3 = std::make_shared<DummyHandler>(3);

    // 添加各种类型的路由
    router.add_route(HttpMethod::GET, "/api/users", handler1);         // 简单路由
    router.add_route(HttpMethod::POST, "/api/posts/:postId", handler2); // 参数化路由
    router.add_route(HttpMethod::GET, "/files/documents/*", handler3); // 通配符路由

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 第一次查找 - 应该需要常规路由匹配
    auto start_time = std::chrono::high_resolution_clock::now();
    EXPECT_EQ(router.find_route(HttpMethod::POST, "/api/posts/123", found_handler, params, query_params), 0);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto first_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params["postId"], "123");

    // 清除参数，保留查询参数
    params.clear();

    // 第二次查找同样的路径 - 应该命中缓存，速度更快
    start_time = std::chrono::high_resolution_clock::now();
    EXPECT_EQ(router.find_route(HttpMethod::POST, "/api/posts/123", found_handler, params, query_params), 0);
    end_time = std::chrono::high_resolution_clock::now();
    auto second_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params["postId"], "123");

    // 验证第二次查找缓存命中，理论上应该比第一次快
    // 由于测试运行环境不确定性，仅打印信息而不硬编码比较
    std::cout << "First lookup (POST /api/posts/123): " << first_duration << " ns" << std::endl;
    std::cout << "Second lookup (cached POST /api/posts/123): " << second_duration << " ns" << std::endl;
    if (second_duration > 0) { // Avoid division by zero
      std::cout << "Cache speedup: "
                << (double)first_duration / second_duration << "x faster"
                << std::endl;
    }


    // 测试通配符路由缓存
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/files/documents/report.pdf", found_handler, params, query_params),
              0);
    EXPECT_EQ(found_handler->id(), 3);
    EXPECT_EQ(params["*"], "report.pdf");

    // 再次查找，应命中缓存
    params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/files/documents/report.pdf", found_handler, params, query_params),
              0);
    EXPECT_EQ(found_handler->id(), 3);
    EXPECT_EQ(params["*"], "report.pdf");

    // 测试缓存清除
    router.clear_cache();

    // 清除缓存后，应该需要重新匹配
    params.clear();
    start_time = std::chrono::high_resolution_clock::now();
    EXPECT_EQ(router.find_route(HttpMethod::POST, "/api/posts/123", found_handler, params, query_params), 0);
    end_time = std::chrono::high_resolution_clock::now();
    auto after_clear_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params["postId"], "123");

    std::cout << "After cache clear (POST /api/posts/123): " << after_clear_duration << " ns" << std::endl;
}

// 测试路由缓存在高压场景下的性能
TEST(RouterTest, CachePerformanceUnderLoad)
{
    http_router<DummyHandler> router;

    // 添加大量路由
    const int NUM_ROUTES = 500;
    std::vector<std::shared_ptr<DummyHandler>> handlers;

    for (int i = 0; i < NUM_ROUTES; i++) {
        handlers.push_back(std::make_shared<DummyHandler>(i));
        HttpMethod method = static_cast<HttpMethod>(i % 4); // GET, POST, PUT, DELETE
        // 混合各类路由
        if (i % 4 == 0) {
            router.add_route(method, "/api/users/" + std::to_string(i), handlers[i]);
        } else if (i % 4 == 1) {
            router.add_route(method, "/api/posts/" + std::to_string(i) + "/:id", handlers[i]);
        } else if (i % 4 == 2) {
            router.add_route(method, "/api/products/" + std::to_string(i) + "/*", handlers[i]);
        } else {
            router.add_route(method, "/static/" + std::to_string(i), handlers[i]);
        }
    }

    // 选择一些路径进行压力测试
    std::vector<std::pair<HttpMethod, std::string>> test_paths_with_methods = {
        {HttpMethod::GET, "/api/users/100"},              // 100 % 4 == 0 -> GET
        {HttpMethod::POST, "/api/posts/101/123"},         // 101 % 4 == 1 -> POST
        {HttpMethod::PUT, "/api/products/102/some/deep/path"}, // 102 % 4 == 2 -> PUT
        {HttpMethod::DELETE, "/static/103"}               // 103 % 4 == 3 -> DELETE
    };


    // 执行多次查找，模拟高频访问
    const int ITERATIONS = 1000;
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 记录总体时间
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; i++) {
        // 循环使用测试路径
        const auto& path_pair = test_paths_with_methods[i % test_paths_with_methods.size()];
        params.clear();
        query_params.clear();

        // 执行路由查找
        EXPECT_EQ(router.find_route(path_pair.first, path_pair.second, found_handler, params, query_params), 0);

        // 第一次后应该都会命中缓存
        if (i >= test_paths_with_methods.size()) {
            // 确保找到了处理程序
            ASSERT_NE(found_handler, nullptr);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    std::cout << "Performed " << ITERATIONS << " lookups in " << total_duration << " ms"
              << std::endl;
    std::cout << "Average: " << (double)total_duration / ITERATIONS << " ms per lookup"
              << std::endl;
}


// Test suite for HTTP method specific functionalities
TEST(HttpRouterMethodTest, AddAndFindWithDifferentMethods) {
    http_router<DummyHandler> router;
    auto handler_get = std::make_shared<DummyHandler>(1);
    auto handler_post = std::make_shared<DummyHandler>(2);
    auto handler_put = std::make_shared<DummyHandler>(3);

    router.add_route(HttpMethod::GET, "/resource", handler_get);
    router.add_route(HttpMethod::POST, "/resource", handler_post);
    router.add_route(HttpMethod::PUT, "/another", handler_put);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Find GET /resource
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/resource", found_handler, params, query_params), 0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 1);

    // Find POST /resource
    found_handler = nullptr; // Reset
    params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::POST, "/resource", found_handler, params, query_params), 0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 2);

    // Find PUT /another
    found_handler = nullptr; // Reset
    params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::PUT, "/another", found_handler, params, query_params), 0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 3);

    // Try to find GET /resource with POST method (should fail or get POST handler depending on strictness, current setup implies separate)
    // Current behavior: Should not find if method mismatches for the specific path storage.
    EXPECT_EQ(router.find_route(HttpMethod::PUT, "/resource", found_handler, params, query_params), -1);

    // Try to find non-existent method for a path
    EXPECT_EQ(router.find_route(HttpMethod::DELETE, "/resource", found_handler, params, query_params), -1);

    // Try to find non-existent path for an existing method
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/nonexistent", found_handler, params, query_params), -1);
}

TEST(HttpRouterMethodTest, MethodSpecificCache) {
    http_router<DummyHandler> router;
    auto handler_get_user = std::make_shared<DummyHandler>(10);
    auto handler_post_user = std::make_shared<DummyHandler>(11);

    router.add_route(HttpMethod::GET, "/user/profile", handler_get_user);
    router.add_route(HttpMethod::POST, "/user/profile", handler_post_user);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // First lookup GET (populates cache for GET:/user/profile)
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/user/profile", found_handler, params, query_params), 0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 10);

    // First lookup POST (populates cache for POST:/user/profile)
    found_handler = nullptr;
    EXPECT_EQ(router.find_route(HttpMethod::POST, "/user/profile", found_handler, params, query_params), 0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 11);

    // Second lookup GET (should hit cache for GET:/user/profile)
    found_handler = nullptr;
    // std::cout << "Finding GET /user/profile again" << std::endl;
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/user/profile", found_handler, params, query_params), 0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 10); // Should be GET handler

    // Second lookup POST (should hit cache for POST:/user/profile)
    found_handler = nullptr;
    // std::cout << "Finding POST /user/profile again" << std::endl;
    EXPECT_EQ(router.find_route(HttpMethod::POST, "/user/profile", found_handler, params, query_params), 0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 11); // Should be POST handler

    // Clear cache and verify
    router.clear_cache();
    found_handler = nullptr;
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/user/profile", found_handler, params, query_params), 0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 10); // Should still be GET handler, but not from cache initially
}

TEST(HttpRouterMethodTest, AddRouteWithUnknownMethod)
{
    http_router<DummyHandler> router;
    auto handler = std::make_shared<DummyHandler>(1);
    // Adding with UNKNOWN method should ideally be a no-op or an error.
    // Current add_route implementation has a check: if (path.empty() || !handler || method == HttpMethod::UNKNOWN) return;
    router.add_route(HttpMethod::UNKNOWN, "/test_unknown", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    EXPECT_EQ(router.find_route(HttpMethod::UNKNOWN, "/test_unknown", found_handler, params, query_params), -1);
    // Also try finding with a valid method, it shouldn't be there.
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/test_unknown", found_handler, params, query_params), -1);
}


int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
