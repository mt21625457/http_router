#include "http_router.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

class DummyHandler
{
public:
    DummyHandler() = default;
    ~DummyHandler() = default;

    explicit DummyHandler(int id) : id_(id) {}
    int id() const { return id_; }

private:
    int id_ = 0;
};

// 测试混合路由策略基本功能
TEST(HybridRoutingTest, BasicHybridRouting)
{
    http_router<DummyHandler> router;

    // 添加不同类型的路由
    auto h1 = std::make_shared<DummyHandler>(1);
    auto h2 = std::make_shared<DummyHandler>(2);
    auto h3 = std::make_shared<DummyHandler>(3);

    // 这些路由应该被存储在哈希表中
    router.add_route(HttpMethod::GET, "/api", h1);   // 短路径
    router.add_route(HttpMethod::GET, "/about", h1); // 短路径
    router.add_route(HttpMethod::GET, "/login", h1); // 短路径

    // 这些路由应该被存储在Trie树中
    router.add_route(HttpMethod::GET, "/api/users/profiles/settings", h2);         // 长路径
    router.add_route(HttpMethod::GET, "/api/users/profiles/photos", h2);           // 长路径
    router.add_route(HttpMethod::GET, "/api/users/profiles/friends/requests", h2); // 长路径

    // 这些应该被存储为参数化路由
    router.add_route(HttpMethod::GET, "/users/:userId", h3);              // 参数化路由
    router.add_route(HttpMethod::GET, "/api/posts/:postId/comments", h3); // 参数化路由
    router.add_route(HttpMethod::GET, "/files/*", h3);                    // 通配符路由

    // 测试参数
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试哈希表路由
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/api", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 1);

    // 测试Trie树路由
    params.clear();
    EXPECT_EQ(
        router.find_route(HttpMethod::GET, "/api/users/profiles/settings", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 2);

    // 测试参数化路由
    params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/users/42", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 3);
    EXPECT_EQ(params["userId"], "42");
}

// 测试混合路由系统的效率
TEST(HybridRoutingTest, RoutingEfficiency)
{
    http_router<DummyHandler> router;

    // 添加大量路由以测试效率
    const int NUM_ROUTES = 1000;
    std::vector<std::shared_ptr<DummyHandler>> handlers;

    for (int i = 0; i < NUM_ROUTES; i++) {
        handlers.push_back(std::make_shared<DummyHandler>(i));
    }

    // 添加短路径 (哈希表)
    for (int i = 0; i < 200; i++) {
        std::string path = "/short" + std::to_string(i);
        router.add_route(HttpMethod::GET, path, handlers[i]);
    }

    // 添加长路径 (Trie树)
    for (int i = 200; i < 500; i++) {
        std::string path = "/api/users/profiles/settings/" + std::to_string(i);
        router.add_route(HttpMethod::GET, path, handlers[i]);
    }

    // 添加参数化路由
    for (int i = 500; i < 800; i++) {
        std::string path = "/users/" + std::to_string(i) + "/:id";
        router.add_route(HttpMethod::GET, path, handlers[i]);
    }

    // 添加通配符路由
    for (int i = 800; i < NUM_ROUTES; i++) {
        std::string path = "/files/" + std::to_string(i) + "/*";
        router.add_route(HttpMethod::GET, path, handlers[i]);
    }

    // 测试参数
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试哈希表路由查找性能
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 200; i++) {
        std::string path = "/short" + std::to_string(i);
        router.find_route(HttpMethod::GET, path, found_handler, params, query_params);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto hash_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    // 测试Trie树路由查找性能
    start_time = std::chrono::high_resolution_clock::now();
    for (int i = 200; i < 500; i++) {
        std::string path = "/api/users/profiles/settings/" + std::to_string(i);
        router.find_route(HttpMethod::GET, path, found_handler, params, query_params);
    }
    end_time = std::chrono::high_resolution_clock::now();
    auto trie_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    // 测试参数化路由查找性能
    start_time = std::chrono::high_resolution_clock::now();
    for (int i = 500; i < 800; i++) {
        std::string path = "/users/" + std::to_string(i) + "/123";
        router.find_route(HttpMethod::GET, path, found_handler, params, query_params);
    }
    end_time = std::chrono::high_resolution_clock::now();
    auto param_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    std::cout << "Hash table lookup: " << hash_time / 200.0 << " μs per lookup" << std::endl;
    std::cout << "Trie tree lookup: " << trie_time / 300.0 << " μs per lookup" << std::endl;
    std::cout << "Parameterized lookup: " << param_time / 300.0 << " μs per lookup" << std::endl;

    // 哈希表路由应该最快，然后是Trie树，最后是参数化路由
    EXPECT_LE(hash_time / 200.0, trie_time / 300.0);
    EXPECT_LE(trie_time / 300.0, param_time / 300.0);
}

// 测试路由查找优先级
TEST(HybridRoutingTest, RoutingPriority)
{
    http_router<DummyHandler> router;

    // 创建路由处理程序
    auto static_handler = std::make_shared<DummyHandler>(1);
    auto param_handler = std::make_shared<DummyHandler>(2);

    // 添加静态和参数化路由到同一路径模式
    router.add_route(HttpMethod::GET, "/api/users", static_handler);    // 静态路由
    router.add_route(HttpMethod::GET, "/api/:resource", param_handler); // 参数化路由

    // 测试参数
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试路由匹配优先级 - 静态路由应该优先于参数化路由
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/api/users", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 1); // 应该匹配静态路由

    // 测试参数化路由
    params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/api/products", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 2); // 应该匹配参数化路由
    EXPECT_EQ(params["resource"], "products");
}

// 测试路径段数匹配优化效果
TEST(HybridRoutingTest, SegmentIndexingOptimization)
{
    http_router<DummyHandler> router;

    // 创建很多带有不同段数的参数化路由
    auto handler1 = std::make_shared<DummyHandler>(1);
    auto handler2 = std::make_shared<DummyHandler>(2);
    auto handler3 = std::make_shared<DummyHandler>(3);

    // 1段路由
    router.add_route(HttpMethod::GET, "/:param", handler1);

    // 2段路由
    for (int i = 0; i < 100; i++) {
        router.add_route(HttpMethod::GET, "/test" + std::to_string(i) + "/:param", handler1);
    }

    // 3段路由
    for (int i = 0; i < 100; i++) {
        router.add_route(HttpMethod::GET, "/api/:resource/:id" + std::to_string(i), handler2);
    }

    // 4段路由
    for (int i = 0; i < 100; i++) {
        router.add_route(HttpMethod::GET, "/api/v1/:resource/:id" + std::to_string(i), handler3);
    }

    // 测试参数
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 测试3段路由匹配效率 - 应该直接查找3段路由
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 20; i++) {
        params.clear();
        EXPECT_EQ(router.find_route(HttpMethod::GET, "/api/users/id" + std::to_string(i), found_handler, params,
                                    query_params),
                  0);
        EXPECT_EQ(found_handler->id(), 2);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto segment_optimized_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    std::cout << "Segment index optimization: " << segment_optimized_time / 20.0 << " μs per lookup"
              << std::endl;

    // 性能测试，没有硬性断言
}

// 测试长路径前缀共享效率
TEST(HybridRoutingTest, TriePrefixSharing)
{
    http_router<DummyHandler> router;

    // 创建大量具有共同前缀的路由
    const int NUM_ROUTES = 1000;
    std::vector<std::shared_ptr<DummyHandler>> handlers;

    for (int i = 0; i < NUM_ROUTES; i++) {
        handlers.push_back(std::make_shared<DummyHandler>(i));

        // 所有路由共享 "/api/v1/users/profiles" 前缀
        std::string path = "/api/v1/users/profiles/setting" + std::to_string(i);
        router.add_route(HttpMethod::GET, path, handlers[i]);
    }

    // 测试参数
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 随机访问一些路由，测量性能
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, NUM_ROUTES - 1);

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; i++) {
        int idx = distrib(gen);
        std::string path = "/api/v1/users/profiles/setting" + std::to_string(idx);
        router.find_route(HttpMethod::GET, path, found_handler, params, query_params);
        EXPECT_EQ(found_handler->id(), idx);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto trie_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    std::cout << "Trie tree with shared prefixes: " << trie_time / 100.0 << " μs per lookup"
              << std::endl;

    // 仅限性能测试，不进行硬性断言
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}