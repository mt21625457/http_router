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
    router.add_route("/test1", handler1);
    router.add_route("/test2", handler2);
    router.add_route("/test/nested/path", handler3);

    // Test finding existing routes
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    EXPECT_EQ(router.find_route("/test1", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler1);

    EXPECT_EQ(router.find_route("/test2", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler2);

    EXPECT_EQ(router.find_route("/test/nested/path", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler3);

    // Test finding non-existent route
    EXPECT_EQ(router.find_route("/nonexistent", found_handler, params, query_params), -1);
}

TEST(RouterTest, ParameterExtraction)
{
    http_router<DummyHandler> router;
    auto handler = std::make_shared<DummyHandler>();

    // Add a route with a parameter
    router.add_route("/users/:id", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test parameter extraction
    EXPECT_EQ(router.find_route("/users/123", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler);
    EXPECT_EQ(params["id"], "123");
}

TEST(RouterTest, MultipleParameters)
{
    http_router<DummyHandler> router;
    auto handler = std::make_shared<DummyHandler>();

    // Add a route with multiple parameters
    router.add_route("/users/:userId/posts/:postId", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test multiple parameter extraction
    EXPECT_EQ(router.find_route("/users/123/posts/456", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler);
    EXPECT_EQ(params["userId"], "123");
    EXPECT_EQ(params["postId"], "456");
}

TEST(RouterTest, WildcardWithParameters)
{
    http_router<DummyHandler> router;
    auto handler = std::make_shared<DummyHandler>();

    // Add a route with wildcard and parameters
    router.add_route("/files/:path/*", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test wildcard with parameter extraction
    EXPECT_EQ(router.find_route("/files/documents/report.pdf", found_handler, params, query_params),
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
    router.add_route("/search", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test basic query parameter extraction
    EXPECT_EQ(router.find_route("/search?q=test&page=2", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler);
    EXPECT_EQ(query_params["q"], "test");
    EXPECT_EQ(query_params["page"], "2");

    // Clear params for next test
    params.clear();
    query_params.clear();

    // Test URL-encoded query parameters
    EXPECT_EQ(router.find_route("/search?q=hello+world&filter=category%3Dbooks", found_handler,
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
    router.add_route("/users/:userId/posts/:postId", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test combination of path and query parameters
    EXPECT_EQ(router.find_route("/users/123/posts/456?sort=date&order=desc", found_handler, params,
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
    router.add_route("/options", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test query parameters without values
    EXPECT_EQ(router.find_route("/options?debug&verbose", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler, handler);
    EXPECT_EQ(query_params["debug"], "");
    EXPECT_EQ(query_params["verbose"], "");

    // Test mixed query parameters with and without values
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route("/options?debug&level=info", found_handler, params, query_params),
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
    router.add_route("/api", short_handler); // 短路径 -> 哈希表
    router.add_route("/api/users/profiles/settings/notifications",
                     long_handler);                             // 长路径 -> Trie树
    router.add_route("/products/:category/:id", param_handler); // 参数化路由

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试短路径查找 (哈希表)
    EXPECT_EQ(router.find_route("/api", found_handler, params, query_params), 0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 1);

    // 测试长路径查找 (Trie树)
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route("/api/users/profiles/settings/notifications", found_handler, params,
                                query_params),
              0);
    ASSERT_NE(found_handler, nullptr);
    EXPECT_EQ(found_handler->id(), 2);

    // 测试参数化路由查找
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route("/products/electronics/12345", found_handler, params, query_params),
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
    router.add_route("/", handlers[0]);
    router.add_route("/home", handlers[1]);
    router.add_route("/about", handlers[2]);
    router.add_route("/contact", handlers[3]);
    router.add_route("/login", handlers[4]);
    router.add_route("/signup", handlers[5]);

    // 长路径路由 (Trie树)
    router.add_route("/api/v1/users/profiles/settings", handlers[10]);
    router.add_route("/api/v1/users/profiles/photos", handlers[11]);
    router.add_route("/api/v1/users/profiles/friends", handlers[12]);
    router.add_route("/api/v1/posts/recent/comments", handlers[13]);
    router.add_route("/api/v1/posts/trending/today", handlers[14]);

    // 参数化路由
    router.add_route("/users/:userId", handlers[20]);
    router.add_route("/users/:userId/profile", handlers[21]);
    router.add_route("/users/:userId/posts/:postId", handlers[22]);
    router.add_route("/products/:category/:productId/reviews", handlers[23]);

    // 通配符路由
    router.add_route("/static/*", handlers[30]);
    router.add_route("/files/:type/*", handlers[31]);


    // 测试各种路由查找
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试通配符
    EXPECT_EQ(router.find_route("/static/123", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 30);
    EXPECT_EQ(params["*"], "123");

    // 测试短路径
    EXPECT_EQ(router.find_route("/about", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 2);

    // 测试长路径
    params.clear();
    query_params.clear();
    EXPECT_EQ(
        router.find_route("/api/v1/posts/trending/today", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 14);

    // 测试参数化路由
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route("/users/42/posts/123", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 22);
    EXPECT_EQ(params["userId"], "42");
    EXPECT_EQ(params["postId"], "123");

    // 测试通配符路由
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route("/files/documents/reports/annual/2023.pdf", found_handler, params,
                                query_params),
              0);
    EXPECT_EQ(found_handler->id(), 31);
    EXPECT_EQ(params["type"], "documents");
    EXPECT_EQ(params["*"], "reports/annual/2023.pdf");

    // 测试混合参数和查询参数
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route("/products/electronics/12345/reviews?sort=newest&page=2",
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
    router.add_route("/api/users", handler1);
    router.add_route("/api/:resource", handler2); // 可能与静态路由冲突
    router.add_route("/api/users/:id", handler3); // 更具体的路由

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试静态路由优先于参数化路由
    EXPECT_EQ(router.find_route("/api/users", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 1); // 应该匹配静态路由

    // 测试参数化路由匹配其他资源
    params.clear();
    EXPECT_EQ(router.find_route("/api/products", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params["resource"], "products");

    // 测试更具体的参数化路由
    params.clear();
    EXPECT_EQ(router.find_route("/api/users/42", found_handler, params, query_params), 0);
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

        // 添加各种类型的路由
        if (i % 5 == 0) {
            // 短路径
            router.add_route("/short" + std::to_string(i), handlers[i]);
        } else if (i % 5 == 1) {
            // 长路径
            router.add_route("/api/v1/users/profiles/settings/" + std::to_string(i), handlers[i]);
        } else if (i % 5 == 2) {
            // 参数化路由
            router.add_route("/users/" + std::to_string(i) + "/:param", handlers[i]);
        } else if (i % 5 == 3) {
            // 带多个参数的路由
            router.add_route("/products/:category/" + std::to_string(i) + "/:id", handlers[i]);
        } else {
            // 通配符路由
            router.add_route("/files/" + std::to_string(i) + "/*", handlers[i]);
        }
    }

    // 执行一些路由匹配以验证性能
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试短路径查找（哈希表）
    EXPECT_EQ(router.find_route("/short100", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 100);

    // 测试长路径查找（Trie树）
    params.clear();
    EXPECT_EQ(router.find_route("/api/v1/users/profiles/settings/101", found_handler, params,
                                query_params),
              0);
    EXPECT_EQ(found_handler->id(), 101);

    // 测试参数化路由
    params.clear();
    EXPECT_EQ(router.find_route("/users/102/test", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 102);
    EXPECT_EQ(params["param"], "test");

    // 测试通配符路由
    params.clear();
    EXPECT_EQ(
        router.find_route("/files/104/some/deep/path.txt", found_handler, params, query_params), 0);
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
    router.add_route("/api/users", handler1);         // 简单路由
    router.add_route("/api/posts/:postId", handler2); // 参数化路由
    router.add_route("/files/documents/*", handler3); // 通配符路由

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 第一次查找 - 应该需要常规路由匹配
    auto start_time = std::chrono::high_resolution_clock::now();
    EXPECT_EQ(router.find_route("/api/posts/123", found_handler, params, query_params), 0);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto first_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params["postId"], "123");

    // 清除参数，保留查询参数
    params.clear();

    // 第二次查找同样的路径 - 应该命中缓存，速度更快
    start_time = std::chrono::high_resolution_clock::now();
    EXPECT_EQ(router.find_route("/api/posts/123", found_handler, params, query_params), 0);
    end_time = std::chrono::high_resolution_clock::now();
    auto second_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params["postId"], "123");

    // 验证第二次查找缓存命中，理论上应该比第一次快
    // 由于测试运行环境不确定性，仅打印信息而不硬编码比较
    std::cout << "First lookup: " << first_duration << " ns" << std::endl;
    std::cout << "Second lookup (cached): " << second_duration << " ns" << std::endl;
    std::cout << "Cache speedup: "
              << (first_duration > 0 ? (double)first_duration / second_duration : 0) << "x faster"
              << std::endl;

    // 测试通配符路由缓存
    params.clear();
    query_params.clear();
    EXPECT_EQ(router.find_route("/files/documents/report.pdf", found_handler, params, query_params),
              0);
    EXPECT_EQ(found_handler->id(), 3);
    EXPECT_EQ(params["*"], "report.pdf");

    // 再次查找，应命中缓存
    params.clear();
    EXPECT_EQ(router.find_route("/files/documents/report.pdf", found_handler, params, query_params),
              0);
    EXPECT_EQ(found_handler->id(), 3);
    EXPECT_EQ(params["*"], "report.pdf");

    // 测试缓存清除
    router.clear_cache();

    // 清除缓存后，应该需要重新匹配
    params.clear();
    start_time = std::chrono::high_resolution_clock::now();
    EXPECT_EQ(router.find_route("/api/posts/123", found_handler, params, query_params), 0);
    end_time = std::chrono::high_resolution_clock::now();
    auto after_clear_duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params["postId"], "123");

    std::cout << "After cache clear: " << after_clear_duration << " ns" << std::endl;
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
        // 混合各类路由
        if (i % 4 == 0) {
            router.add_route("/api/users/" + std::to_string(i), handlers[i]);
        } else if (i % 4 == 1) {
            router.add_route("/api/posts/" + std::to_string(i) + "/:id", handlers[i]);
        } else if (i % 4 == 2) {
            router.add_route("/api/products/" + std::to_string(i) + "/*", handlers[i]);
        } else {
            router.add_route("/static/" + std::to_string(i), handlers[i]);
        }
    }

    // 选择一些路径进行压力测试
    std::vector<std::string> test_paths = {"/api/users/100", "/api/posts/101/123",
                                           "/api/products/102/some/deep/path", "/static/103"};

    // 执行多次查找，模拟高频访问
    const int ITERATIONS = 1000;
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 记录总体时间
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; i++) {
        // 循环使用测试路径
        const std::string &path = test_paths[i % test_paths.size()];
        params.clear();

        // 执行路由查找
        EXPECT_EQ(router.find_route(path, found_handler, params, query_params), 0);

        // 第一次后应该都会命中缓存
        if (i >= test_paths.size()) {
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

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
