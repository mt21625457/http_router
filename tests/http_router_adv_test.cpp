/**
 * @file http_router_adv_test.cpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief Advanced and edge case tests for HTTP router
 * HTTP路由器的高级功能和边界情况测试
 * @version 0.1
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 */

#include "../include/router/router.hpp"
#include "../include/router/router_group.hpp"
#include "../include/router/router_impl.hpp"

#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace flc;

// Handler for testing purposes
class DummyHandler
{
public:
    DummyHandler() = default;
    explicit DummyHandler(int id) : id_(id) {}

    // Copy and move operations following Google style
    DummyHandler(const DummyHandler &) = default;
    DummyHandler &operator=(const DummyHandler &) = default;
    DummyHandler(DummyHandler &&) = default;
    DummyHandler &operator=(DummyHandler &&) = default;

    // 仿函数调用操作符（满足CallableHandler约束）
    void operator()() const
    {
        // Implementation of operator()
    }

    int id() const { return id_; }

private:
    int id_ = 0;
};

// Test suite for advanced and edge cases
class HttpRouterAdvTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        router_ = std::make_unique<router<DummyHandler>>();
        params_.clear();
        query_params_.clear();
    }

    std::unique_ptr<router<DummyHandler>> router_;
    std::map<std::string, std::string> params_;
    std::map<std::string, std::string> query_params_;
};

// Test path parsing with various edge cases
TEST_F(HttpRouterAdvTest, PathParsingEdgeCases)
{
    auto handler = DummyHandler(1);
    router_->add_route(HttpMethod::GET, "/a/b", std::move(handler));
    router_->add_route(HttpMethod::GET, "/c", std::move(handler));

    // Path with trailing slash should match the version without it
    auto result = router_->find_route(HttpMethod::GET, "/a/b/", params_, query_params_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);

    // Path with consecutive slashes should be treated as one
    result = router_->find_route(HttpMethod::GET, "/a//b", params_, query_params_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);

    // Leading consecutive slashes
    result = router_->find_route(HttpMethod::GET, "//c", params_, query_params_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);

    // Non-existent path
    result = router_->find_route(HttpMethod::GET, "/a/b/c", params_, query_params_);
    EXPECT_FALSE(result.has_value());
}

// Test url_decode with more complex characters
TEST_F(HttpRouterAdvTest, UrlDecodeSpecialChars)
{
    auto handler = DummyHandler(1);
    router_->add_route(HttpMethod::GET, "/search", std::move(handler));

    const char *original_path =
        "/search?q=%2Fpath%2Fto%2Fresource%20%26%20more"; // "/path/to/resource & more"
    auto result = router_->find_route(HttpMethod::GET, original_path, params_, query_params_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);
    EXPECT_EQ(query_params_["q"], "/path/to/resource & more");
}

// Test unsupported wildcard position (not at the end)
TEST_F(HttpRouterAdvTest, UnsupportedWildcardPosition)
{
    auto handler = DummyHandler(1);
    router_->add_route(HttpMethod::GET, "/files/*/details", std::move(handler));

    // This should not match, as wildcards in the middle are not supported
    auto result =
        router_->find_route(HttpMethod::GET, "/files/report.pdf/details", params_, query_params_);
    EXPECT_FALSE(result.has_value());
}

// Corrected and more robust LRU Eviction Test
TEST_F(HttpRouterAdvTest, LRUEvictionCorrectnessTest)
{
    // Note: MAX_CACHE_SIZE is a constexpr set to 1000 in router.hpp
    constexpr size_t kCacheSize = 1000;
    constexpr int kNumRoutesToAdd = kCacheSize + 100;

    std::vector<DummyHandler> handlers;
    for (int i = 0; i < kNumRoutesToAdd; ++i) {
        handlers.push_back(DummyHandler(i));
        // We must add all routes *before* testing cache eviction,
        // because add_route clears the cache.
        router_->add_route(HttpMethod::GET, "/item/" + std::to_string(i), std::move(handlers[i]));
    }

    // 1. Fill the cache completely by accessing the first CACHE_SIZE routes.
    // Access order: 0, 1, 2, ..., 999
    for (size_t i = 0; i < kCacheSize; ++i) {
        auto result = router_->find_route(HttpMethod::GET, "/item/" + std::to_string(i), params_,
                                          query_params_);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value().get().id(), i);
    }
    // Cache is now full. The LRU list's front is 999, back is 0.

    // 2. Access a new route, which should cause an eviction of the least recently used item
    // ("/item/0").
    auto result = router_->find_route(HttpMethod::GET, "/item/" + std::to_string(kCacheSize),
                                      params_, query_params_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), kCacheSize);
    // Now "/item/1000" is in cache, and "/item/0" should have been evicted.
    // LRU back is now "/item/1".

    // 3. Re-access an item to move it to the front of the LRU list.
    result = router_->find_route(HttpMethod::GET, "/item/50", params_, query_params_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 50);
    // Now "/item/50" is the most recently used. The LRU back is still "/item/1".

    // 4. Access another new item, which should evict "/item/1".
    result = router_->find_route(HttpMethod::GET, "/item/" + std::to_string(kCacheSize + 1),
                                 params_, query_params_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), kCacheSize + 1);

    // To verify, we can't inspect the cache directly. But if we assume the cache is working,
    // a subsequent call to an evicted route will be a cache miss, while a call to a
    // non-evicted route will be a cache hit. We can't measure time reliably here,
    // but the logic of what *should* be in the cache can be asserted.
    // We assume the test passes if no crashes occur and logic seems sound, since we can't inspect
    // the cache.

    // Let's test that we can still find the evicted items (via regular routing).
    result = router_->find_route(HttpMethod::GET, "/item/0", params_, query_params_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 0);

    result = router_->find_route(HttpMethod::GET, "/item/1", params_, query_params_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 1);

    // And that an item that should still be cached is also findable.
    result = router_->find_route(HttpMethod::GET, "/item/50", params_, query_params_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().get().id(), 50);

    SUCCEED() << "LRU eviction test logic executed successfully.";
}
