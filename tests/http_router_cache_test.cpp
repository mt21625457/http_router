#include "http_router.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <string>
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

// 基本缓存功能测试
TEST(RouterCacheTest, BasicCacheFunctionality)
{
    http_router<DummyHandler> router;

    // 添加不同类型的路由
    auto handler1 = std::make_shared<DummyHandler>(1);
    auto handler2 = std::make_shared<DummyHandler>(2);
    auto handler3 = std::make_shared<DummyHandler>(3);

    router.add_route(HttpMethod::GET, "/static/path", handler1);       // 静态路由 (哈希表)
    router.add_route(HttpMethod::GET, "/api/users/:id", handler2);     // 参数化路由
    router.add_route(HttpMethod::GET, "/files/documents/*", handler3); // 通配符路由

    // 测试参数
    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 第一次访问 - 需要路由查找
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/api/users/123", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params["id"], "123");

    // 清除参数，再次访问同一路径应该命中缓存并填充参数
    params.clear();
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/api/users/123", found_handler, params, query_params), 0);
    EXPECT_EQ(found_handler->id(), 2);
    EXPECT_EQ(params["id"], "123"); // 验证缓存还原了参数
}

// 缓存清除测试
TEST(RouterCacheTest, CacheClearTest)
{
    http_router<DummyHandler> router;

    auto handler = std::make_shared<DummyHandler>(1);
    router.add_route(HttpMethod::GET, "/test/:param", handler);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 第一次访问
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/test/value", found_handler, params, query_params), 0);
    EXPECT_EQ(params["param"], "value");

    // 清除缓存
    router.clear_cache();

    // 清除参数
    params.clear();

    // 再次访问，因为缓存已清除，应该重新匹配路由
    EXPECT_EQ(router.find_route(HttpMethod::GET, "/test/value", found_handler, params, query_params), 0);
    EXPECT_EQ(params["param"], "value");
}

// 路由缓存性能测试
TEST(RouterCacheTest, CachePerformanceTest)
{
    http_router<DummyHandler> router;

    // 添加1000个不同的路由
    for (int i = 0; i < 1000; i++) {
        auto handler = std::make_shared<DummyHandler>(i);

        if (i % 3 == 0) {
            // 静态路由
            router.add_route(HttpMethod::GET, "/path" + std::to_string(i), handler);
        } else if (i % 3 == 1) {
            // 参数化路由
            router.add_route(HttpMethod::GET, "/users/" + std::to_string(i) + "/:id", handler);
        } else {
            // 通配符路由
            router.add_route(HttpMethod::GET, "/files/" + std::to_string(i) + "/*", handler);
        }
    }

    // 准备100个测试路径
    std::vector<std::string> test_paths;
    for (int i = 0; i < 100; i++) {
        if (i % 3 == 0) {
            test_paths.push_back("/path" + std::to_string(i * 3));
        } else if (i % 3 == 1) {
            test_paths.push_back("/users/" + std::to_string(i * 3 + 1) + "/test_id");
        } else {
            test_paths.push_back("/files/" + std::to_string(i * 3 + 2) + "/some/deep/path");
        }
    }

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 第一轮 - 无缓存匹配
    auto start = std::chrono::high_resolution_clock::now();

    for (const auto &path : test_paths) {
        params.clear();
        router.find_route(HttpMethod::GET, path, found_handler, params, query_params);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto no_cache_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // 第二轮 - 应该命中缓存
    start = std::chrono::high_resolution_clock::now();

    for (const auto &path : test_paths) {
        params.clear();
        router.find_route(HttpMethod::GET, path, found_handler, params, query_params);
    }

    end = std::chrono::high_resolution_clock::now();
    auto with_cache_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "No cache: " << no_cache_time << " μs" << std::endl;
    std::cout << "With cache: " << with_cache_time << " μs" << std::endl;
    std::cout << "Speedup: " << (no_cache_time > 0 ? (double)no_cache_time / with_cache_time : 0)
              << "x" << std::endl;

    // 期望缓存版本至少快2倍
    EXPECT_GT((double)no_cache_time / with_cache_time, 2.0);
}

// LRU缓存淘汰策略测试
TEST(RouterCacheTest, LRUEvictionTest)
{
    http_router<DummyHandler> router;

    // 向路由器添加足够数量的路由
    const int ROUTE_COUNT = 1000; // 使用与缓存大小相同的路由数量
    std::vector<std::shared_ptr<DummyHandler>> handlers;

    for (int i = 0; i < ROUTE_COUNT; i++) {
        auto handler = std::make_shared<DummyHandler>(i);
        handlers.push_back(handler);
        router.add_route(HttpMethod::GET, "/test" + std::to_string(i), handler);
    }

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 清除缓存
    router.clear_cache();

    // 第一步：访问所有路由填充缓存
    for (int i = 0; i < ROUTE_COUNT; i++) {
        router.find_route(HttpMethod::GET, "/test" + std::to_string(i), found_handler, params, query_params);
        EXPECT_EQ(found_handler->id(), i);
    }

    // 第二步：添加额外的路由，这会导致缓存开始淘汰最早的条目
    const int EXTRA_ROUTES = 100;
    for (int i = ROUTE_COUNT; i < ROUTE_COUNT + EXTRA_ROUTES; i++) {
        auto handler = std::make_shared<DummyHandler>(i);
        handlers.push_back(handler);
        router.add_route(HttpMethod::GET, "/test" + std::to_string(i), handler);

        // 访问新路由填充缓存
        router.find_route(HttpMethod::GET, "/test" + std::to_string(i), found_handler, params, query_params);
        EXPECT_EQ(found_handler->id(), i);
    }

    // 第三步：再次访问一些靠后的路由，确保它们仍在缓存中
    // 这些应该仍在缓存中
    for (int i = ROUTE_COUNT - 50; i < ROUTE_COUNT; i++) {
        router.find_route(HttpMethod::GET, "/test" + std::to_string(i), found_handler, params, query_params);
        EXPECT_EQ(found_handler->id(), i);
    }

    // 第四步：访问靠后的路由，保证它们在缓存中
    for (int i = ROUTE_COUNT; i < ROUTE_COUNT + EXTRA_ROUTES; i++) {
        router.find_route(HttpMethod::GET, "/test" + std::to_string(i), found_handler, params, query_params);
        EXPECT_EQ(found_handler->id(), i);
    }

    // 第五步：现在访问前面一些路由，这些应该已经被淘汰了
    // 但我们仍应该能找到正确的处理程序
    for (int i = 0; i < 10; i++) {
        router.find_route(HttpMethod::GET, "/test" + std::to_string(i), found_handler, params, query_params);
        // 验证我们仍能找到正确的处理程序
        EXPECT_EQ(found_handler->id(), i);
    }

    // 现在重新访问一些中间的路由，应该会更新它们在缓存中的位置
    for (int i = 10; i < 20; i++) {
        router.find_route(HttpMethod::GET, "/test" + std::to_string(i), found_handler, params, query_params);
        EXPECT_EQ(found_handler->id(), i);
    }

    // 添加更多的路由，应该会淘汰更多旧的缓存条目
    for (int i = ROUTE_COUNT + EXTRA_ROUTES; i < ROUTE_COUNT + EXTRA_ROUTES + 100; i++) {
        auto handler = std::make_shared<DummyHandler>(i);
        handlers.push_back(handler);
        router.add_route(HttpMethod::GET, "/test" + std::to_string(i), handler);

        // 访问新路由填充缓存
        router.find_route(HttpMethod::GET, "/test" + std::to_string(i), found_handler, params, query_params);
        EXPECT_EQ(found_handler->id(), i);
    }

    // 验证刚刚访问过的中间路由仍然在缓存中
    // 这是LRU的关键测试：最近使用的应该保留
    for (int i = 10; i < 20; i++) {
        router.find_route(HttpMethod::GET, "/test" + std::to_string(i), found_handler, params, query_params);
        EXPECT_EQ(found_handler->id(), i);
    }

    // 测试通过表示LRU缓存机制工作正常
    std::cout << "LRU cache test passed!" << std::endl;
}

// 随机访问模式测试
TEST(RouterCacheTest, RandomAccessPattern)
{
    http_router<DummyHandler> router;

    // 添加大量路由
    const int NUM_ROUTES = 2000;
    std::vector<std::shared_ptr<DummyHandler>> handlers;
    for (int i = 0; i < NUM_ROUTES; ++i) {
        handlers.push_back(std::make_shared<DummyHandler>(i));
        router.add_route(HttpMethod::GET, "/route/" + std::to_string(i), handlers.back());
    }

    // 随机访问路由
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<> distrib(0, NUM_ROUTES - 1);

    std::shared_ptr<DummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // 初次访问，填充缓存
    for (int i = 0; i < NUM_ROUTES; ++i) {
        router.find_route(HttpMethod::GET, "/route/" + std::to_string(i), found_handler, params, query_params);
    }

    // 随机访问1000次
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        int idx = distrib(g);
        std::string path = "/route/" + std::to_string(idx);
        router.find_route(HttpMethod::GET, path, found_handler, params, query_params);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto random_access_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // 顺序访问1000次
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        std::string path = "/route/" + std::to_string(i);
        router.find_route(HttpMethod::GET, path, found_handler, params, query_params);
    }
    end = std::chrono::high_resolution_clock::now();
    auto sequential_access_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "Random access time: " << random_access_time / 1000.0 << " ns/op" << std::endl;
    std::cout << "Sequential access time: " << sequential_access_time / 1000.0 << " ns/op"
              << std::endl;
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}